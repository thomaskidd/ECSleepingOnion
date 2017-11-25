#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include <ugpio/ugpio.h>

// GPIO INTIALIZATION FUNCTION

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

int mdelay = 500; 				// set frequency of main loop in milliseconds
const int udelay = mdelay*1000; // convert to microseconds

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

	// DECLARATION OF TIMER
	time_t  progTime; 					// time in seconds
	struct tm * contents;

	time(&progTime);					// get time in seconds
	contents = localtime(&progTime);	// convert to local time

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
	gpioUsed1 = intializeGPIO(gpio1, 0);
	if(!gpioUsed1)
	{
		fprintf(pLogFile, "@ %s   FATAL: Pin %d failed to initialize, ending program.\r\n", asctime(contents), gpio1);
		return EXIT_FAILURE;
	}

	// initialize second pin
	gpio2 = atoi(argv[2]);
	gpioUsed2 = intializeGPIO(gpio2, 0);
	if(!gpioUsed2)
	{
		fprintf(pLogFile, "@ %s   FATAL: Pin %d failed to initialize\r\n", asctime(contents), gpio2);
		return EXIT_FAILURE;
	}

	///////////////////////////////////////////////////////////
	// MAIN LOOP (should be based on an actual timer not sleep)
	///////////////////////////////////////////////////////////

	int inBed = 0;
	int done = 0;
	// int currentTime = 0;
	printf("Beginning test loop...\n");
	while(!done)
	{
		// this is used for logging / data
		time(&progTime);					// get time in seconds
		contents = localtime(&progTime);	// convert to local time

		// get value from pressure sensor; a value of one is inBed
		gpioValue1 = gpio_get_value(gpio1);

		if(gpioValue1 && !inBed)
		{
			// print time getting into bed as the first part of the csv
			// prints like asctime but without the /n at the end, and with weekday/month as number
			fprintf(pDataFile, "%d %.2d %.2d %.2d:%.2d:%.2d, ", contents->tm_wday, contents->tm_mon,
					contents->tm_mday, contents->tm_hour, contents->tm_min, contents->tm_sec);
			inBed = 1;
		} else if(!gpioValue1 && inBed)
		{
			// print time getting out of bed as the second part of the csv
			fprintf(pDataFile,  "%d %.2d %.2d %.2d:%.2d:%.2d\r\n", contents->tm_wday, contents->tm_mon,
					contents->tm_mday, contents->tm_hour, contents->tm_min, contents->tm_sec);
			inBed = 0;
		}

		printf("inBed: %d\n",inBed);

		// finish loop if second pin is activated
		gpioValue2 = gpio_get_value(gpio2);
		if(gpioValue2)
		{
			printf("Pin %d was activated, ending program.\n", gpio2);
			fprintf(pLogFile, "@ %s   INFO: Pin %d was activated, ending program.\r\n", asctime(contents), gpio2);
			done = 1;
		}

		// currentTime++;
		usleep(delay);
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
	// close the log file
	fclose(pLogFile);
	fclose(pDataFile);

	return 0;
}


