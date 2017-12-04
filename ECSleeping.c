#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#include <ugpio/ugpio.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include <errno.h>
#include <ctype.h>
#include <getopt.h>



// GPIO INTIALIZATION FUNCTION

const struct tm maxTime = {0, 0, 0, 1, 0, 200, 0, 0, -1};
const struct tm minTime = {0, 0, 0, 1, 0, 2, 0, 0, -1};

// pin initialization as seen in gpioRead on the Onion git, just put in a function
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



//atoi
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


//Converts the four digits recieved to from the phone into readable time
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


// function to write to the log file during the loop (this way it saves it in case it crashes)
void logOut(char message[], struct tm * logTime)
{
	FILE *pLogFile;
	pLogFile = fopen("/root/ECSleeping/Logs/week1.log", "a");

	if(pLogFile == NULL)
	{
		perror("Warning: Log file did not open.");
	}

	fprintf(pLogFile, "@ %s   %s", asctime(logTime), message);

	fclose(pLogFile);
}



// function to write to the data file during the loop (again most of the data will be retained if it exits improperly)
void dataOut(struct tm firstTime, struct tm * secondTime)
{
	FILE *pDataFile;
	pDataFile = fopen("/root/ECSleeping/Data/week1.csv", "a");

	if(pDataFile == NULL)
	{
		perror("Error: Data file failed to open.");
		logOut("ERROR: Data file failed to open.", secondTime);
	} else
	{
		// print time getting into bed as the first part of the csv
		// prints like asctime but without the /n at the end, and with weekday/month as number
		fprintf(pDataFile, "%d %.2d %.2d %.2d:%.2d:%.2d, ", firstTime.tm_wday, firstTime.tm_mon,
				firstTime.tm_mday, firstTime.tm_hour, firstTime.tm_min, firstTime.tm_sec);

		// print time getting out of bed as the second part of the csv
		fprintf(pDataFile,  "%d %.2d %.2d %.2d:%.2d:%.2d\r\n", secondTime->tm_wday, secondTime->tm_mon,
				secondTime->tm_mday, secondTime->tm_hour, secondTime->tm_min, secondTime->tm_sec);
	}

	fclose(pDataFile);
}


/**********************************************************************
*    str2uuid referenced from :
*    http://www.humbug.in/2010/sample-bluetooth-rfcomm-client-app-in-c
*	@author : P. Sinah
**********************************************************************/
//convverts a string into a uuid for the phone to read
int str2uuid( const char *uuid_str, uuid_t *uuid ) 
{
    uint32_t uuid_int[4];
    char *endpointer;

    if( strlen( uuid_str ) == 36 ) {
    
        //parse uuid128 standard format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
        char buf[9] = { 0 };
		
		//bluetooth is little endian, make sure the uuid_int32 is stored in this format (host to network long: htonl() )
		
		//check if dashes are at valid indexes in the string
        if( uuid_str[8] != '-' && uuid_str[13] != '-' &&
            uuid_str[18] != '-'  && uuid_str[23] != '-' ) {
            return 0;
        }
        
        //parse first 8 bytes of uuid
        strncpy(buf, uuid_str, 8);
        uuid_int[0] = htonl( strtoul( buf, &endpointer, 16 ) );
        if( endpointer != buf + 8 ) {
        	return 0;
		}
		
        //parse second 8 bytes of uuid
        strncpy(buf, uuid_str+9, 4);
        strncpy(buf+4, uuid_str+14, 4);
        uuid_int[1] = htonl( strtoul( buf, &endpointer, 16 ) );
        if( endpointer != buf + 8 ) {
        	return 0;
        }

        //parse third 8 bytes of uuid
        strncpy(buf, uuid_str+19, 4);
        strncpy(buf+4, uuid_str+24, 4);
        uuid_int[2] = htonl( strtoul( buf, &endpointer, 16 ) );
        if( endpointer != buf + 8 ) {
        	return 0;
		}
		
        //parse fourth 8 bytes of uuid
        strncpy(buf, uuid_str+28, 8);
        uuid_int[3] = htonl( strtoul( buf, &endpointer, 16 ) );
        if( endpointer != buf + 8 ) {
        	return 0;
        }

        if( uuid != NULL ) {
        	sdp_uuid128_create( uuid, uuid_int );
        }
        
    } 
    
    else {
        return 0;
    }

    return 1;
}
/**********************************************************************
*    End Reference
**********************************************************************/

//Compares two tm structs and returns 1 if they are exactly the same
int isSameTime(struct tm time1, struct tm time2)
{
	int same = 0;
	
	if(time1.tm_sec == time2.tm_sec && time1.tm_min == time2.tm_min && time1.tm_hour == time2.tm_hour && time1.tm_mday == time2.tm_mday && time1.tm_mon == time2.tm_mon && 
		time1.tm_year == time2.tm_year && time1.tm_wday == time2.tm_wday && time1.tm_yday == time2.tm_yday && time1.tm_isdst == time1.tm_isdst)
	{
		same = 1;
	}
	
	return same;
}





int main(int argc, char **argv, char **envp)
{
	// DECLARATION OF TIMERS

	// initiliaze date and time tracking for data / log files
	time_t progTime; 					// time in seconds
	time_t startTime;
	struct tm * contents;
	struct tm timeInBed;

	time(&progTime);					// get time in seconds
	startTime = progTime;				// convert to local time
	contents = localtime(&progTime);

	// initialize the clock timers for loop timer and update time
	clock_t clockTime = clock();
	clock_t lastConnectionCheck = clock(); 	// the last time checking the connection
	clock_t lastPinCheck = clock();			// the last time checking the pins

	const double connectionCheckTime = 0.2;	// in the loop check the bluetooth connection every x seconds
	const double pinCheckTime = 0.5;		// in the loop check the pin reading every x seconds

	double connectionDiffTime;
	double pinDiffTime;



	// initialize pointer for log file
	FILE *pLogFile;
	pLogFile = fopen("/root/ECSleeping/Logs/week1.log", "w");

	if(pLogFile == NULL)
	{
		perror("Error: Log file failed to open.");
		exit(-1);
	}

	// initialize pointer for data file
	FILE *pDataFile;
	pDataFile = fopen("/root/ECSleeping/Data/week1.csv", "w");

	if(pDataFile == NULL)
	{
		perror("Error: Data file failed to open.");
		exit(-1);
	}

	if(argc < 3)
	{
		perror("\nError: Insufficient parameters...");
		printf("   Usage: ./ECSleeping <gpio1> <gpio2>\n");
		printf("   gpio1 reads sensor, gpio2 ends program\n");
		exit(-1);
	}




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




	int sendStatus = -1;
	int timeBytes = 0;
	char buf[8];

	struct tm nextAlarm = maxTime;  // alarm set for 100 years in the future (alarm will not go off)
	struct tm lastAlarm = minTime;  // previous alarm went off a long time ago (no need to check if sleeping yet)
	int alarmGoing = 0;			





	///////////////////////////////////
	//BLUETOOTH

	//scanning variables and structs
	int i = 0; //loop variable
	int j = 0; //loop variable
	int sock = 0; //specified socket for searching
	int dev_id = -1; //bluetooth adapter id
	int foundName = 0; //boolean, has the search found the target device

	struct hci_dev_info dev_info;
	inquiry_info *info = NULL;
	int num_rsp= 0; //number of responses from bluetooth discovery
	int length = 8; //scan for 1.28 * 8s
	int flags = 0; //optjions for scanning, currently set to scan for all previous devices in the cache
	bdaddr_t ba; //bluetooth address struct
	char addr[19] = { 0 }; //tmp address string
	char name[248] = { 0 }; //tmp name string
	char targetAddr[19] = { 0 }; //MAC address of target device
	char targetName[248] = { 0 }; //user friendly name of target device
	uuid_t uuid = { 0 }; //128bit uuid, this will be created using str2uuid


	//sdp query and connection variables and structs
	int infoCounter = 0; //index of inquiry_info array which holds the info object for the target device
	int error = 0; //return value from sdp service search request
	char *uuid_str="00001101-0000-1000-8000-00805F9B34FB";
	uint32_t range = 0x0000ffff; //range of uuids to check
	sdp_list_t *response_list = NULL; //responses returned by sdp query
	sdp_list_t *search_list; //sdp list that holds the uuid
	sdp_list_t *attrid_list; //sdp list that holds the range
	int s = 0; //return value from opening a socket
	int local_channel = -1; //local channel on the remote device
	int status = 0; //status returned by write command
	struct sockaddr_rc loc_addr = { 0 }; //socket addres struct to connect to the remote device
	bdaddr_t tmpAddr; //tmp bluetooth address struct
	int retries = 0; //number of connection retries
	int found = 0; //used as boolean if uuid is found in an SDP query
	int responses = 0; //number of responses in SDP query
	sdp_session_t *session; //SDP session variable

	//use default signal handling when receiving an interrupt
	(void) signal(SIGINT, SIG_DFL);
	
	//get a handle on the local bluetooth adapter
	dev_id = hci_get_route(NULL);
	if (dev_id < 0) {
		perror("No Bluetooth Adapter Available");
		return -1;
	}
	
	//get information about the local device
	if (hci_devinfo(dev_id, &dev_info) < 0) {
		perror("Can't get device info");
		return -1;
	}


	//try to open hci bluetootu adapter
	sock = hci_open_dev( dev_id );
	if (sock < 0) {
		perror("HCI device open failed");
		free(info);
		return -1;
	}
	
	//convert our uuid string to a uuid_t type
	if( !str2uuid( uuid_str, &uuid ) ) {
		perror("Invalid UUID");
		free(info);
		return -1;
	}
	
	//keep searching until the taret device is found
  	while(!foundName) {

		printf("Scanning for target device ...\n");
		info = NULL;
		num_rsp = 0;
		flags = 0; //still scan for devices even if Omega2 is not sure if they're in range
		length = 8; /* ~10 seconds */
		
		//scan for available devices
		num_rsp = hci_inquiry(dev_id, length, num_rsp, NULL, &info, flags);
		
		//try again if inquiry fails
		if (num_rsp < 0) {
			perror("Inquiry failed");
			continue;
		}

		//check info structs returned by hci_inquiry
		for (int i = 0; i < num_rsp; i++) {
			
			ba2str(&(info+i)->bdaddr, addr); //copy the MAC address of the discovered device to the tmp address
			memset(name, 0, sizeof(name)); //set the tmp name to null
			
			//get the friendly name from the remote device's address
			if (hci_read_remote_name(sock, &(info+i)->bdaddr, sizeof(name), name, 0) < 0) {
				strcpy(name, "[unknown]");
			}

			printf("Found %s  %s\n", addr, name);

			if (strcmp(name, "Yusef's G5") == 0) {
				printf("Success\n");
				infoCounter = i;
				foundName = 1;
				strcpy(targetName, name);
				strcpy(targetAddr, addr);
			}
		}
	}

  	//attempt to connect to the app's sdp service
  	session = 0;
  	responses = 0;
    while(!session) {

        session = sdp_connect( BDADDR_ANY, &(info+infoCounter)->bdaddr, SDP_RETRY_IF_BUSY );

        if(session) {
        	printf("Connected to sdp\n");
    		break;
		}
		
		//wait and retry again
        if(errno == EALREADY && retries < 5) {
            perror("Retrying");
            retries++;
            sleep(1);
            continue;
        }
    }

    //call error if session is null
    if ( session == NULL ) {
        perror("Can't open session with the device");
        free(info);
    }

    //set up further sdp search variables
    search_list = sdp_list_append( 0, &uuid );
    attrid_list = sdp_list_append( 0, &range );
    error = 0;
    error = sdp_service_search_attr_req( session, search_list, SDP_ATTR_REQ_RANGE, attrid_list, &response_list);
    sdp_list_t *resp = response_list; //tmp pointer to response_list
    sdp_record_t *rec;

    //check sdp service records
    for (; resp; resp= resp->next ) {
        responses++;
        rec = (sdp_record_t*) resp->data;
        sdp_list_t *proto_list;
        
        //get a list of the protocol sequences
        if( sdp_get_access_protos( rec, &proto_list ) == 0 ) {
        sdp_list_t *p = proto_list;

            //go through each protocol sequence
            for( ; p ; p = p->next ) {
                    sdp_list_t *pds = (sdp_list_t*)p->data;

                    //go through each protocol list of the protocol sequence
                    for( ; pds ; pds = pds->next ) {

                            //check the protocol attributes
                            sdp_data_t *d = (sdp_data_t*)pds->data;
                            int proto = 0;
                            for( ; d; d = d->next ) {
                                    switch( d->dtd ) { 
										case SDP_UUID16:
										case SDP_UUID32:
										case SDP_UUID128:
											proto = sdp_uuid_to_proto( &d->val.uuid );
											break;
										case SDP_UINT8:
											if( proto == RFCOMM_UUID ) {
												printf("rfcomm channel: %d\n",d->val.int8);
												local_channel = d->val.int8;
												found = 1;
											}
											break;
                                    }
                            }
                    }
                    sdp_list_free( (sdp_list_t*)p->data, 0 );
            }
            sdp_list_free( proto_list, 0 );

        }
        if (local_channel > 0)
            break;

	}
	
	//Opens/creates the socket
	if ( local_channel > 0 && found == 1 ) {
        //printf("Found service on this device, now gonna blast it with dummy data\n");
        s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
        loc_addr.rc_family = AF_BLUETOOTH;
        loc_addr.rc_channel = loca;_channel;
        loc_addr.rc_bdaddr = *(&(info+infoCounter)->bdaddr);
        status = connect(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
        if( status < 0 ) {
            perror("uh oh");
        }
	}
	
	// close the files in order to save them, we'll append them in the logOut and dataOut functions later
	fclose(pLogFile);
	fclose(pDataFile);

	///////////////////////////////////////////////////////////
	// MAIN LOOP (based on clock time to not rely on busy wait)
	///////////////////////////////////////////////////////////

	int inBed = 0;
	int done = 0; // done can be triggered either by gpio2 or after 24 hours
	printf("Beginning main loop...\n");
	while(!done)
	{
		// this is used for logging / data
		time(&progTime);					// get time in seconds
		contents = localtime(&progTime);	// convert to local time

		// check if 24 hours has passed
		if(difftime(progTime, startTime) >= 60*60*24)
			done = 1;

		clockTime = clock();				// get current clock cycle

		// PIN CHECKING AND DATA WRITING
		pinDiffTime = (double)(clockTime - lastPinCheck)/CLOCKS_PER_SEC; // get difference in time

		if(pinDiffTime < 0)
		{
			// this probably just means the clock_t var has ticked over, reseting to 0
			logOut("INFO: Clock overflowed, skipping current time check.\r\n", contents);
			lastPinCheck = clock();

		}
		else if(pinDiffTime >= pinCheckTime)
		{
			// get value from pressure sensor; a value of one is inBed
			gpioValue1 = gpio_get_value(gpio1);

			if(gpioValue1 && !inBed)
			{
				// set time gotten into bed
				timeInBed = *contents;
				inBed = 1;
				printf("inBed: %d\n",inBed);
			} else if(!gpioValue1 && inBed)
			{
				// use data out to write time in bed , time out of bed 
				dataOut(timeInBed, contents);
				inBed = 0;
				printf("inBed: %d\n",inBed);
			}
			// update the last pin check
			lastPinCheck = clock();
		}



		clockTime = clock();


		// BLUETOOTH CONNECTING AND COMMUNICATION
		connectionDiffTime = (double)(clockTime - lastConnectionCheck)/CLOCKS_PER_SEC; // get difference in time

		if(connectionDiffTime < 0)
		{
			// this probably just means the clock_t var has ticked over, reseting to 0
			logOut("INFO: Clock overflowed, skipping current time check.\r\n", contents);
			lastConnectionCheck = clock();
		}
		else if (connectionDiffTime >= connectionCheckTime)
		{

			// read time from phone
			//read from phone
			timeBytes = recv(s, buf, sizeof(buf), O_NONBLOCK);
			
			if(timeBytes > 0)
			{
				nextAlarm = stringToTime(buf);
			}

			if( sendStatus < 0 )
			{
				logOut("WARNING: Data failed to send to server\n", contents);	//logs connection error
				//connected = -1; // attempt to recconnect
			}
			
			
			
			// send command
			if(alarmGoing)
			{
				//turn off alarm if user wakes up
				if(!inBed)
				{
					sendStatus = write(s, "0", 1); //"1": set off alarm		"0": alarm off
					alarmGoing = 0;
					
					lastAlarm = *contents;

					if( sendStatus < 0 )
					{
						logOut("WARNING: Data failed to send to server\n", contents);	//logs connection error
						//connected = -1; // attempt to recconnect
					}
				}
			}
			else
			{
				//sets off alarm when programed time is reached
				if(inBed && difftime(mktime(&nextAlarm), progTime) < 1 && difftime(mktime(&nextAlarm), progTime) > -1 && !isSameTime(nextAlarm, maxTime))
				{
					sendStatus = write(s, "1", 1); //"1": set off alarm		"0": alarm off
					alarmGoing = 1;
					
					if( sendStatus < 0 )
					{
						logOut("WARNING: Data failed to send to server\n", contents);	//logs connection error
						//connected = -1; // attempt to recconnect
					}
				}
				if(inBed && difftime(progTime, mktime(&lastAlarm)) < 300 && !isSameTime(lastAlarm, minTime)) //sets off alarm if back in bed within 5 minutes
				{
					sendStatus = write(s, "1", 1); //"1": set off alarm		"0": alarm off
					alarmGoing = 1;
					
					if( sendStatus < 0 )
					{
						logOut("WARNING: Data failed to send to server\n", contents);	//logs connection error
						//connected = -1; // attempt to recconnect
					}
				}
				
			}

			// update the last connection check
			lastConnectionCheck = clock();
		}		
		
		
		
		// finish loop if second pin is activated
		gpioValue2 = gpio_get_value(gpio2);
		if(gpioValue2)
		{
			printf("Pin %d was activated, ending program.\n", gpio2);
			logOut("INFO: Deactivation pin was activated, ending program.\r\n", contents);
			done = 1;
		}

	}

	////////////////////
	// END OF MAIN LOOP 
	////////////////////

	// CLOSING DOWN THE SYSTEM

	FILE *pDownLogFile;
	pLogFile = fopen("/root/ECSleeping/Logs/week1.log", "a");

	if(pDownLogFile == NULL)
	{
		perror("Warning: Log file did not open.");
	}

	// unexport the gpios
	if(gpioUsed1)
	{
		printf("> unexporting gpio%d\n", gpio1);
		if (gpio_free(gpio1) < 0)
		{
			fprintf(pDownLogFile, "@%s   WARNING: Pin%d is still free.\r\n", asctime(contents), gpio1);
			perror("gpio_free");
		}
	}
	if(gpioUsed2)
	{
		printf("> unexporting gpio%d\n", gpio2);
		if (gpio_free(gpio2) < 0)
		{
			fprintf(pDownLogFile, "@%s   WARNING: Pin%d is still free.\r\n", asctime(contents), gpio2);
			perror("gpio_free");
		}
	}
	
	
	//close the socket
	close(s);
	
	// close the final log file
	fclose(pDownLogFile);

	return 0;
}