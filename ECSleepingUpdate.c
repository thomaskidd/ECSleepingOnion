#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include <ugpio/ugpio.h>

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>




// GPIO INTIALIZATION FUNCTION

const struct tm maxTime = {0, 0, 0, 1, 0, 200, 0, 0, 0};
const struct tm minTime = {0, 0, 0, 1, 0, 2, 0, 0, 0};

int intializeGPIO(const int pin)
{

	int rq;
	int rv;
	// check if gpio is already exported
	rq = gpio_is_requested(pin);
	if(rq < 0)
	{
		perror("gpio_is_requested");
		return 0;
	}
	// export the gpio
	if(!rq)
	{
		printf("> exporting gpio\n");
		rv = gpio_request(pin, NULL);
		if(rv < 0)
		{
			perror("gpio_request");
			return 0;
		}
	}

	printf("Pin %d: initialized.\n\n", pin);
	if(!rq)
		return 1;
}




int myAtoi(char a)
{
	switch(a)
	{
		case '0':
			return 0;
		case '1':
			return 1;
		case '2':
			return 2;
		case '3':
			return 3;
		case '4':
			return 4;
		case '5':
			return 5;
		case '6':
			return 6;
		case '7':
			return 7;
		case '8':
			return 8;
		case '9':
			return 9;
		default:
			return -1;
	}
}




struct tm stringToTime(char* string)
{
	char bigHour = string[0];
	char littleHour = string[1];
	char bigMinute = string[2];
	char littleMinute = string[3];
	
	int temp;
	int hour;
	int minute;
	
	hour = myAtoi(bigHour) * 10;
	temp = myAtoi(littleHour);
	
	if(hour < 0 || temp < 0)
	{
		return maxTime;
	}
	
	hour += temp;
	
	if(hour > 23)
	{
		return maxTime;
	}
	
	minute = myAtoi(bigMinute) * 10;
	temp = myAtoi(littleMinute);
	
	if(minute < 0 || temp < 0)
	{
		return maxTime;
	}
	
	minute += temp;
	
	if(minute > 59)
	{
		return maxTime;
	}
	
	time_t now;
	struct tm* nowTime;
	
	time(&now);
	nowTime = localtime(&now);
	
	struct tm nextTime = *nowTime;
	
	nextTime.tm_hour = hour;
	nextTime.tm_min = minute;
	
	if(difftime(mktime(nowTime), mktime(&nextTime)) > 0)
	{
		nextTime.tm_mday++;
		mktime(&nextTime);
	}
	
	return nextTime;
}




int main(int argc, char **argv, char **envp)
{

	// initialize pointer for log file
	FILE *pLogFile;
	pLogFile = fopen("/ECSleeping/Logs/week1.log", "w");

	if(pLogFile == NULL)
	{
		perror("Error: Log file failed to open.");
		exit(-1);
	}

	// initialize point for data file
	FILE *pDataFile;
	pDataFile = fopen("/ECSleeping/Data/week1.csv", "w");

	if(pDataFile == NULL)
	{
		perror("Error: Data file failed to open.");
		exit(-1);
	}

	if(argc < 3)
	{
		printf("Usage: ./ECSleeping <gpio1> <gpio2>\n");
		printf("gpio1 reads sensor, gpio2 ends program\n");
		exit(-1);
	}



	// DECLARATION OF TIMERS

	// initiliaze date and time tracking for data / log files
	time_t  progTime; 					// time in seconds
	struct tm * contents;

	time(&progTime);					// get time in seconds
	contents = localtime(&progTime);	// convert to local time

	// initialize the clock timers for loop timer and update time
	clock_t clockTime = clock();
	clock_t lastSave = clock();
	clock_t lastConnectionCheck = clock(); 	// the last time checking the connection
	clock_t lastPinCheck = clock();			// the last time checking the pins

	const double saveTime = 60*60; 			// save files every hour
	const double connectionCheckTime = 0.5;	// in the loop check the bluetooth connection every x seconds
	const double pinCheckTime = 0.5;		// in the loop check the pin reading every x seconds

	double connectionDiffTime;
	double pinDiffTime;

	printf("Program start @: %s\n", asctime(contents));
	fprintf(pLogFile, "@ %s   INFO: Program start.\r\n", asctime(contents));



	// INTIALIZING AND ASSIGNING PINS
	int gpio1;
	int gpio2;
	int gpioUsed1;
	int gpioUsed2;
	int gpioValue1;
	int gpioValue2;

	// initialize first pin
	gpio1 = atoi(argv[1]);
	gpioUsed1 = intializeGPIO(gpio1);
	if(!gpioUsed1)
	{
		fprintf(pLogFile, "@ %s   FATAL: Pin %d failed to initialize, ending program.\r\n", asctime(contents), gpio1);
		return EXIT_FAILURE;
	}

	// initialize second pin
	gpio2 = atoi(argv[2]);
	gpioUsed2 = intializeGPIO(gpio2);
	if(!gpioUsed2)
	{
		fprintf(pLogFile, "@ %s   FATAL: Pin %d failed to initialize\r\n", asctime(contents), gpio2);
		return EXIT_FAILURE;
	}

	


	// BLUETOOTH INITIALIZATION
	
	//initialize scan and connection structs and variables
    inquiry_info *info = NULL;
    int max_rsp, num_rsp;
    int dev_id, sock, len, flags;
    int i;
    char address[19] = { 0 };
    char name[248] = { 0 };
	
	//initialize BT communication variables and structs
	struct sockaddr_rc addr = { 0 };
    int s;
	int connected = -1;
	int sendStatus = -1;
	int timeBytes = 0;
	char buf[8];
    char dest[18] = "1234"; // address of the app for Omega to connect to 
	
	struct tm nextAlarm = maxTime;  // alarm set for 100 years in the future (alarm will not go off)
	struct tm lastAlarm = minTime;  // previous alarm went off a long time ago (no need to check if sleeping yet)
	int alarmGoing = 0;			


	
	//////////////////////////////////////////////////
	// BLUETOOTH SCANNING
	//////////////////////////////////////////////////
	
	int bluetooth = 1;
	//get socket for scanning for bluetooth devices
    dev_id = hci_get_route(NULL);
    sock = hci_open_dev( dev_id );
    if(dev_id < 0 || sock < 0) {
        perror("Failed to open socket");
        fprintf(pLogFile, "%s\n", "@ %s   WARNING: Bluetooth socket failed to open, continueing without bluetooth.\r\n", asctime(contents));
        bluetooth = 0;		//if bluetooth fails to open, don't try to use bluetooth
    }

    if(bluetooth) 
    {
	    //scan for 1.28 * len seconds
	    len  = 8;
	    max_rsp = 255; //find max of 255 bluetooth devices
	    flags = IREQ_CACHE_FLUSH;
	    info = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));

	    num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &info, flags);
	    if( num_rsp < 0 ) perror("hci_inquiry");

	    //testing
	    printf("num_rsp: ");
	    printf("%d\n",num_rsp);

	    for (i = 0; i < num_rsp; i++) {
	        ba2str(&(info+i)->bdaddr, address);
	        memset(name, 0, sizeof(name));

	        if (hci_read_remote_name(sock, &(info+i)->bdaddr, sizeof(name), name, 0) < 0) {
	            strcpy(name, "[unknown]");
		}

		//check if this is the desired device to connect to
		int test = strcmp(name, "ECSleeping");
		printf("test: %d", test);
	        if (test > -3 && test < 3) {
		    printf("Confirmed.\n");
		    ba2str(&(info+i)->bdaddr, dest);
		}

	        printf("%s  [%s]\n", address, name);
	    }

	    free( info );
	    close( sock );


	    //connect to selected device
	    if (strcmp(dest,"1234") == 0) {
		printf("Copying error\n");
		return 0;
	    }
		
		// allocate a socket
	    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	}



	///////////////////////////////////////////////////////////
	// MAIN LOOP (based on clock time to not rely on busy wait)
	///////////////////////////////////////////////////////////

	int inBed = 0;
	int done = 0;
	printf("Beginning test loop...\n");
	while(!done)
	{
		// this is used for logging / data
		time(&progTime);					// get time in seconds
		contents = localtime(&progTime);	// convert to local time

		clockTime = clock();				// get current clock cycle

		// SAVING THE FILES EVERY HOUR
		if((double)(clockTime - lastSave)/CLOCKS_PER_SEC)
		{
			fprint(pLogFile, "@ %s   INFO: Saving files...\r\n", asctime(contents));

			// close the files
			fclose(pLogFile);
			fclose(pDataFile);

			// reopening the files with "a" to append
			pLogFile = fopen("/ECSleeping/Logs/week1.log", "a");
			pDataFile = fopen("ECSleeping/Data/week1.csv", "a");

			// making sure they reopened successfully
			if(pLogFile == NULL)
			{
				perror("Error: Log file failed to reopen, will try again next time.");
			} else if(pDataFile == NULL)
			{
				fprintf(pLogFile, "@ %s   INFO: Log file successfully reopened.\r\n", asctime(contents));
				fprintf(pLogFile, "@ %s   WARNING: Data file failed to reopen, will try again next time.\r\n", asctime(contents));
			} else 
			{
				fprintf(pLogFile, "@ %s   INFO: Files successfully reopened.\r\n", asctime(contents));
				lastSave = clock(); // if either of the files fail it will try to save/reopen again next cycle
			}
			
		}



		// PIN CHECKING AND DATA WRITING
		pinDiffTime = (double)(clockTime - lastPinCheck)/CLOCKS_PER_SEC; // get difference in time

		if(pinDiffTime < 0)
		{
			// this probably just means the clock_t var has ticked over, reseting to 0
			fprintf(pLogFile, "@ %s   INFO: Clock overflowed, skipping current time check.\r\n", asctime(contents));
			lastPinCheck = clock();

		}
		else if( pinDiffTime >= pinCheckTime && bluetooth)
		{
			// get value from pressure sensor; a value of one is inBed
			gpioValue1 = gpio_get_value(gpio1);

			if(gpioValue1 && !inBed)
			{
				// print time getting into bed as the first part of the csv
				// prints like asctime but without the /n at the end, and with weekday/month as number
				fprintf(pDataFile, "%d %.2d %.2d %.2d:%.2d:%.2d, ", contents->tm_wday, contents->tm_mon,
						contents->tm_mday, contents->tm_hour, contents->tm_min, contents->tm_sec);
				inBed = 1;
				printf("inBed: %d\n",inBed);
			} else if(!gpioValue1 && inBed)
			{
				// print time getting out of bed as the second part of the csv
				fprintf(pDataFile,  "%d %.2d %.2d %.2d:%.2d:%.2d\r\n", contents->tm_wday, contents->tm_mon,
						contents->tm_mday, contents->tm_hour, contents->tm_min, contents->tm_sec);
				inBed = 0;
				printf("inBed: %d\n",inBed);
			}
		}



		clockTime = clock();




		// BLUETOOTH CONNECTING AND COMMUNICATION
		connectionDiffTime = (double)(clockTime - lastConnectionCheck)/CLOCKS_PER_SEC; // get difference in time

		if(connectionDiffTime < 0)
		{
			// this probably just means the clock_t var has ticked over, reseting to 0
			fprintf(pLogFile, "@ %s   INFO: Clock overflowed, skipping current time check.\r\n", asctime(contents));
			lastConnectionCheck = clock();
		}
		else if (connectionDiffTime >= connectionCheckTime)
		{
			if(connected < 0)
			{
				// set the connection parameters (who to connect to)
				addr.rc_family = AF_BLUETOOTH;
				addr.rc_channel = (uint8_t) 1;
				str2ba( dest, &addr.rc_bdaddr );

				// connect to server
				connected = connect(s, (struct sockaddr *)&addr, sizeof(addr));
			}


			// read time from phone
			if( connected == 0 )
			{
				timeBytes = read(s, buf, sizeof(buf));
				
				if(timeBytes > 0)
				{
					nextAlarm = stringToTime(buf);
				}
				
				// check connection
				sendStatus = write(s, "0", 1); //"1": set off alarm		"0": alarm off
				alarmGoing = 0;
				
				lastAlarm = *contents;

				if( sendStatus < 0 )
				{
					fprintf(pLogFile, "@ %s   WARNING: Data failed to send to server\n", asctime(contents));	//logs connection error
					connected = -1; // attempt to recconnect
				}
			}
			
			// send command
			if( connected == 0 ) 
			{
				if(alarmGoing)
				{
					if(!inBed)
					{
						sendStatus = write(s, "0", 1); //"1": set off alarm		"0": alarm off
						alarmGoing = 0;
						
						lastAlarm = *contents;

						if( sendStatus < 0 )
						{
							fprintf(pLogFile, "@ %s   WARNING: Data failed to send to server\n", asctime(contents));	//logs connection error
							connected = -1; // attempt to recconnect
						}
					}
				}
				else
				{
					if(inBed && difftime(mktime(&nextAlarm), progTime) < 3 && difftime(mktime(&nextAlarm), progTime) > -3)
					{
						sendStatus = write(s, "1", 1); //"1": set off alarm		"0": alarm off
						alarmGoing = 1;
						
						if( sendStatus < 0 )
						{
							fprintf(pLogFile, "@ %s   WARNING: Data failed to send to server\n", asctime(contents));	//logs connection error
							connected = -1; // attempt to recconnect
						}
					}
					if(inBed && difftime(progTime, mktime(&lastAlarm)) < 300) //sets off alarm if back in bed within 5 minutes
					{
						sendStatus = write(s, "1", 1); //"1": set off alarm		"0": alarm off
						alarmGoing = 1;
						
						if( sendStatus < 0 )
						{
							fprintf(pLogFile, "@ %s   WARNING: Data failed to send to server\n", asctime(contents));	//logs connection error
							connected = -1; // attempt to recconnect
						}
					}
					
				}
			}
		}		
		
		
		
		// finish loop if second pin is activated
		gpioValue2 = gpio_get_value(gpio2);
		if(gpioValue2)
		{
			printf("Pin %d was activated, ending program.\n", gpio2);
			fprintf(pLogFile, "@ %s   INFO: Pin %d was activated, ending program.\r\n", asctime(contents), gpio2);
			done = 1;
		}

	}

	////////////////////
	// END OF MAIN LOOP 
	////////////////////

	// CLOSING DOWN THE SYSTEM

	// unexport the gpios
	if(gpioUsed1)
	{
		printf("> unexporting gpio%d\n", gpio1);
		if (gpio_free(gpio1) < 0)
		{
			fprintf(pLogFile, "@%s   WARNING: Pin%d is still free.\r\n", asctime(contents), gpio1);
			perror("gpio_free");
		}
	}
	if(gpioUsed2)
	{
		printf("> unexporting gpio%d\n", gpio2);
		if (gpio_free(gpio2) < 0)
		{
			fprintf(pLogFile, "@%s   WARNING: Pin%d is still free.\r\n", asctime(contents), gpio2);
			perror("gpio_free");
		}
	}
	
	if(bluetooth)
	{
		//close BT socket
		close(s);
	}
	
	// close the files
	fclose(pLogFile);
	fclose(pDataFile);

	return 0;
}