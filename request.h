#pragma once
#include<string>
#include"network.h"


class RequestParser{
	int state;
	std::string workingBackBuffer;
	char workingStr[65536];
	int workingIdx;
	bool finished;
	
	std::string methodString;
	std::string url;
	std::string httpVersion;
	std::map<std::string, std::string> headers;
	std::string body;
	
	bool consumeOne(char chr);
	
public:
	RequestParser();
	
	void consume(char* data, int dataLen);
	
	bool isFinished() { return finished; }
	
	Request getRequest();
};