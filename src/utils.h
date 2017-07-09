#pragma once
#include<string>
#include<ctime>


#define memzero(buffer) memset(&buffer, 0, sizeof(buffer))

struct PipePair
{
	int readHead;
	int writeHead;
};

bool fdClosed(int fd);
std::string readFromFile(std::string filename);
std::string unixTimeToString(std::time_t timeVal);
std::time_t stringToUnixTime(std::string str);
std::string generateTagFromStat(std::string filename);
