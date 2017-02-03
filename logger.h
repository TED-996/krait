#pragma once
#include<fstream>
#include<string>
#include<boost/format.hpp>


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
	void log(const std::string str);
	void log(const boost::format fmt);

	void close();
};


void loopTick2Loggers(LoggerOut& log1, LoggerOut& log2);
