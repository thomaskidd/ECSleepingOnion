#include <fstream>
#include <math.h>
#include <ctime>
#include <iostream>
#include <stdlib.h>

using namespace std;

//One Stats object represents a day of stats
class Stats
{
	public:
		int numSleeps;
		tm* ins;
		tm* outs;
		
		//gets time from start of the data collection to the end in seconds
		double getWindowInSecs()
		{
			if(numSleeps == 0)
			{
				return 0;
			}
			return difftime(mktime(&(ins[0])), mktime(&(outs[numSleeps - 1])));
		}
		
		double getTotalSleepTime()
		{
			double total = 0;
			
			for(int i = 0; i < numSleeps; i++)
			{
				total += difftime(mktime(&(outs[i])), mktime(&(ins[i])));
			}
			
			return total;
		}
		
		double getAverageSleepTime()
		{
			return getTotalSleepTime() / (double)numSleeps;
		}
};

//function and class declarations
int myRead(const char filename[], Stats& stats);
int myWrite(const char filename[], Stats statsArray[], int numStats);
bool parseLine(const char line[], tm& start, tm& end);
enum state{ WDAY1, MON1, MDAY1, HOUR1, MIN1, SEC1, WDAY2, MON2, MDAY2, HOUR2, MIN2, SEC2 };
int myAtoI(char a);
void secondsToHours(double totSeconds, int hourMinSec[3]);
void fixTimeInfo(int hms[3]);


//Reads a .csv file and fills the stats object accordingly
int myRead(const char filename[], Stats& stats)
{
	//////////////////////////
	//Check extension
	int charSize = 0;
	while(filename[charSize])
	{
		charSize++;
	}
	
	bool hasCSV = false;
	
	for(int i = 0; i < charSize; i++)
	{
		if(filename[i] == '.')
		{
			if(filename[i + 1] == 'c' && filename[i + 2] == 's' && filename[i + 3] == 'v' && !filename[i + 4])
			{
				hasCSV = true;
			}
			else
			{
				return -1;
			}
		}
	}
	
	char* file;
	if(hasCSV)
	{
		file = new char[charSize + 1];
	}
	else
	{
		file = new char[charSize + 5];
	}
	
	for(int i = 0; i < charSize; i++)
	{
		file[i] = filename[i];
	}
	
	if(hasCSV)
	{
		file[charSize] = 0;
	}
	else
	{
		file[charSize] = '.';
		file[charSize + 1] = 'c';
		file[charSize + 2] = 's';
		file[charSize + 3] = 'v';
		file[charSize + 4] = 0;
	}
	
	const int maxLineLength = 100;
	
	char stuffs[maxLineLength];
	ifstream count;
	count.open(file);
	
	if(!count.is_open())
	{
		return -1;
	}
	
	////////////////////////////////////////////////
	//Reads and parses all lines into a Stats object
	
	stats.ins = new tm[300];	//enough for all 5 minute intervals in a day
	stats.outs = new tm[300];
	
	ifstream in;
	in.open(file);
	if(!in.is_open())
	{
		return -1;
	}
	char line[maxLineLength];
	
	stats.numSleeps = 0;
	bool go = true;
	
	while(!in.eof() && go)
	{
		if(!in.getline(line, maxLineLength))
		{
			if(!in.eof())
			{
				return -1;
			}
		}

		//Parse the line. If a line doesn't conform to the expected format, assume the end of the file has been reached.
		if(parseLine(line, stats.ins[stats.numSleeps], stats.outs[stats.numSleeps]))
		{
			//disregards cases when the time between change is less than 5 minutes
			if(stats.numSleeps != 0 && difftime(mktime(&(stats.outs[stats.numSleeps])), mktime(&(stats.ins[stats.numSleeps]))) < 300)
			{
				//do nothing
				//disregard interval
			}
			else if(stats.numSleeps != 0 && difftime(mktime(&(stats.ins[stats.numSleeps])), mktime(&(stats.outs[stats.numSleeps - 1]))) < 300)
			{
				//Overwrites previous outOfBed value
				stats.outs[stats.numSleeps - 1] = stats.outs[stats.numSleeps];
			}
			else
			{
				stats.numSleeps++;
			}
		}
		else
		{
			go = false;
		}
		
	}
	
	
	return 0;
}


////////////////////////////////////////////////////////////////
//Write out stats for the day in Stats object to "filename.data"
int myWrite(const char filename[], Stats statsArray[], int numStats)
{
	
	//checks extension of file
	int charSize = 0;
	while(filename[charSize])
	{
		charSize++;
	}
	
	bool hasCSV = false;
	
	for(int i = 0; i < charSize; i++)
	{
		if(filename[i] == '.')
		{
			if(filename[i + 1] == 'c' && filename[i + 2] == 's' && filename[i + 3] == 'v' && !filename[i + 4])
			{
				hasCSV = true;
			}
			else
			{
				return -1;
			}
		}
	}
	
	char* file;
	if(hasCSV)
	{
		file = new char[charSize + 2];
	}
	else
	{
		file = new char[charSize + 6];
	}
	
	for(int i = 0; i < charSize; i++)
	{
		file[i] = filename[i];
	}
	
	if(hasCSV)
	{
		file[charSize - 3] = 's';
		file[charSize - 2] = 't';
		file[charSize - 1] = 'a';
		file[charSize] = 't';
		file[charSize + 1] = 0;
	}
	else
	{
		file[charSize] = '.';
		file[charSize + 1] = 's';
		file[charSize + 2] = 't';
		file[charSize + 3] = 'a';
		file[charSize + 4] = 't';
		file[charSize + 5] = 0;
	}
	
	double avgSleepTime = 0;
	
	for(int i = 0; i < numStats; i++)
	{
		avgSleepTime += (statsArray[i].getTotalSleepTime() / (double) numStats);
	}
	
	int avgHMS[3];
	
	secondsToHours(avgSleepTime, avgHMS);
	
	//Can write more types of statistics here later
	
	ofstream out;
	
	out.open(file);
	if(!out.is_open())
	{
		return -1;
	}
	
	out << "Average sleep time per night: ";
	out << avgHMS[0] << " hour(s), " << avgHMS[1] << " minute(s), " << avgHMS[2] << " second(s)\r\n";
	
	out.close();
	return 0;
}



bool parseLine(const char line[], tm& start, tm& end)
{
	int i = 0;
	bool done = 0;
	state spot = WDAY1;
	
	//initialize tm values
	start.tm_wday = 0;
	start.tm_mon = 0;
	start.tm_mday = 0;
	start.tm_hour = 0;
	start.tm_min = 0;
	start.tm_sec = 0;
	start.tm_year = 100;
	end.tm_wday = 0;
	end.tm_mon = 0;
	end.tm_mday = 0;
	end.tm_hour = 0;
	end.tm_min = 0;
	end.tm_sec = 0;
	end.tm_year = 100;
	
	while(!done)
	{
		switch(spot)
		{
			case WDAY1:
				if(line[i] >= 48 && line[i] <= 57 )	//if char is a number
				{
					start.tm_wday *= 10;
					start.tm_wday += myAtoI(line[i]);
					i++;
				}
				else if(line[i] == ' ')
				{
					spot = MON1;
					i++;
				}
				else
				{
					return false;
				}
				break;
			case MON1:
				if(line[i] >= 48 && line[i] <= 57 )	//if char is a number
				{
					start.tm_mon *= 10;
					start.tm_mon += myAtoI(line[i]);
					i++;
				}
				else if(line[i] == ' ')
				{
					spot = MDAY1;
					i++;
				}
				else
				{
					return false;
				}
				break;
			case MDAY1:
				if(line[i] >= 48 && line[i] <= 57 )	//if char is a number
				{
					start.tm_mday *= 10;
					start.tm_mday += myAtoI(line[i]);
					i++;
				}
				else if(line[i] == ' ')
				{
					spot = HOUR1;
					i++;
				}
				else
				{
					return false;
				}
				break;
			case HOUR1:
				if(line[i] >= 48 && line[i] <= 57 )	//if char is a number
				{
					start.tm_hour *= 10;
					start.tm_hour += myAtoI(line[i]);
					i++;
				}
				else if(line[i] == ':')
				{
					spot = MIN1;
					i++;
				}
				else
				{
					return false;
				}
				break;
			case MIN1:
				if(line[i] >= 48 && line[i] <= 57 )	//if char is a number
				{
					start.tm_min *= 10;
					start.tm_min += myAtoI(line[i]);
					i++;
				}
				else if(line[i] == ':')
				{
					spot = SEC1;
					i++;
				}
				else
				{
					return false;
				}
				break;
			case SEC1:
				if(line[i] >= 48 && line[i] <= 57 )	//if char is a number
				{
					start.tm_sec *= 10;
					start.tm_sec += myAtoI(line[i]);
					i++;
				}
				else if(line[i] == ',')
				{
					spot = WDAY2;
					i++;
					i++;	//for the space
				}
				else
				{
					return false;
				}
				break;
			case WDAY2:
				if(line[i] >= 48 && line[i] <= 57 )	//if char is a number
				{
					end.tm_wday *= 10;
					end.tm_wday += myAtoI(line[i]);
					i++;
				}
				else if(line[i] == ' ')
				{
					spot = MON2;
					i++;
				}
				else
				{
					return false;
				}
				break;
			case MON2:
				if(line[i] >= 48 && line[i] <= 57 )	//if char is a number
				{
					end.tm_mon *= 10;
					end.tm_mon += myAtoI(line[i]);
					i++;
				}
				else if(line[i] == ' ')
				{
					spot = MDAY2;
					i++;
				}
				else
				{
					return false;
				}
				break;
			case MDAY2:
				if(line[i] >= 48 && line[i] <= 57 )	//if char is a number
				{
					end.tm_mday *= 10;
					end.tm_mday += myAtoI(line[i]);
					i++;
				}
				else if(line[i] == ' ')
				{
					spot = HOUR2;
					i++;
				}
				else
				{
					return false;
				}
				break;
			case HOUR2:
				if(start.tm_mon == 11 && end.tm_mon == 0)
				{
					end.tm_year = 101;
				}
				if(line[i] >= 48 && line[i] <= 57 )	//if char is a number
				{
					end.tm_hour *= 10;
					end.tm_hour += myAtoI(line[i]);
					i++;
				}
				else if(line[i] == ':')
				{
					spot = MIN2;
					i++;
				}
				else
				{
					return false;
				}
				break;
			case MIN2:
				if(line[i] >= 48 && line[i] <= 57 )	//if char is a number
				{
					end.tm_min *= 10;
					end.tm_min += myAtoI(line[i]);
					i++;
				}
				else if(line[i] == ':')
				{
					spot = SEC2;
					i++;
				}
				else
				{
					return false;
				}
				break;
			case SEC2:
				if(line[i] >= 48 && line[i] <= 57 )	//if char is a number
				{
					start.tm_sec *= 10;
					start.tm_sec += myAtoI(line[i]);
					i++;
				}
				else if(line[i] == '\r' || line[i] == '\n' || !line[i])
				{
					return true;
				}
				else
				{
					return false;
				}
				break;
		}
	}
	return false;
}

int myAtoI(char a)
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

//mutates an array with hours, minutes, and seconds values respectively
void secondsToHours(double totSeconds, int hourMinSec[3])
{
	unsigned long int secs = (unsigned long int) totSeconds;	//cuts off fractions of a second
	
	hourMinSec[0] = secs / 3600;	//total hours
	secs %= 3600;
	
	hourMinSec[1] = secs / 60;		//remaining minutes
	secs %= 60;
	
	hourMinSec[2] = secs;			//remaining seconds
	
}




int main(const int argc, const char* const argv[])
{
	if(argc < 2)
	{
		return -1;
	}
	
	int numDataFiles = 0;
	
	Stats statsArray[argc - 1];
	
	for(int i = 1; i < argc; i++)
	{
		if(!myRead(argv[i], statsArray[numDataFiles]))
		{
			numDataFiles++;
		}
	}
	
	if(myWrite("testData", statsArray, numDataFiles) < 0)
	{
		return -1;
	}
	
	return 0;
}