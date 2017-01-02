#pragma once
#include<fstream>
#include<string>


class LoggerOut {
	int pipeIn;
	std::ofstream outfile;

public:
	LoggerOut(int pipeIn);
	LoggerOut(int pipeIn, std::string filename);

	bool tick(int timeoutMs);

	void close();
};


class LoggerIn {
	int pipeOut;

public:
	LoggerIn(int pipeOut);

	void log(const char* buffer, size_t size);
	void log(const char* str);
	void log(std::string str);

	void close();
};


void autoLogger(LoggerOut& log1, LoggerOut& log2);
