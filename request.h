#pragma once
#include<string>
#include"network.h"


class RequestParser{
	int state;
	std::string workingBackBuffer;
	char workingStr[65536];
	unsigned int workingIdx;
	
	bool finished;
	unsigned long long bodyLeft;
	
	std::string workingHeaderName;
	std::string workingHeaderValue;
	
	std::string methodString;
	std::string url;
	std::string httpVersion;
	std::map<std::string, std::string> headers;
	std::string body;
	
	void saveHeader(std::string name, std::string value);
	
	bool consumeOne(char chr);
	
	int getBodyLength();
	
public:
	RequestParser();
	
	void consume(char* data, int dataLen);
	
	bool isFinished() { return finished; }
	
	Request getRequest();
};