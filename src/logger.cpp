#include<boost/chrono.hpp>
#include<poll.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include"logger.h"

namespace bc = boost::chrono;

LoggerIn Loggers::infoLogger(1);
LoggerIn Loggers::errLogger(2);

void Loggers::setLoggers(int infoPipe, int errPipe) {
	setInfoLogger(infoPipe);
	setErrLogger(errPipe);
}

void Loggers::setInfoLogger(int infoPipe) {
	/*infoLogger.~LoggerIn();
	new(&infoLogger)LoggerIn(infoPipe);*/
	infoLogger = LoggerIn(infoPipe);
}

void Loggers::setErrLogger(int errPipe) {
	/*errLogger.~LoggerIn();
	new(&errLogger)LoggerIn(errPipe);*/
	infoLogger = LoggerIn(errPipe);
}

void Loggers::logInfo(const std::string& message) {
	infoLogger.log(message);
}

void Loggers::logErr(const std::string& message) {
	errLogger.log(message);
}

LoggerOut::LoggerOut() {
	this->pipeIn = -1;
}

LoggerOut::LoggerOut(int pipeIn)
	: outfile() {
	this->pipeIn = pipeIn;
}


LoggerOut::LoggerOut(int pipeIn, const std::string& filename)
	: outfile(filename) {
	this->pipeIn = pipeIn;
}


bool LoggerOut::tick(int timeoutMs) {
	if (pipeIn == -1) {
		fprintf(stderr, "LoggerOut::tick(): Error, invalid FD\n");
		return false;
	}

	auto start = bc::thread_clock::now();
	bc::thread_clock::duration limit = bc::milliseconds(timeoutMs);

	pollfd pfd;
	pfd.fd = pipeIn;
	pfd.events = POLLIN;
	pfd.revents = 0;

	char buffer[4096];
	bool written = false;

	while (bc::thread_clock::now() - start < limit) {
		int pollResult = poll(&pfd, 1, timeoutMs / 10);
		if (pollResult == -1) {
			//Error... ignore for now.
			continue;
		}
		if (pollResult == 1) {
			int bytesRead = read(pipeIn, buffer, 4096);
			if (bytesRead == -1) {
				//Error... ignore for now. Other than that, WHY?
				continue;
			}
			if (bytesRead == 0) {
				return false;
			}
			if (outfile) {
				outfile.write(buffer, bytesRead);
				written = true;
			}
		}
	}

	if (poll(&pfd, 1, 0) == 1) {
		//Log "Can't keep up!"l
	}

	if (written && outfile) {
		outfile.flush();
	}

	return true;
}


void LoggerOut::close() {
	if (pipeIn != -1) {
		::close(pipeIn);
		pipeIn = -1;
	}
	if (outfile) {
		outfile.close();
	}
}


LoggerIn::LoggerIn(int pipeOut) {
	this->pipeOut = dup(pipeOut);
}


void LoggerIn::log(const char* buffer, size_t size) {
	if (pipeOut == -1) {
		fprintf(stderr, "LoggerIn::log: Cannot log; invalid FD attached.\n");
		return;
	}

	if (write(pipeOut, buffer, size) != (int)size || write(pipeOut, "\n", 1) != 1) {
		fprintf(stderr, "Error in LoggerIn.write; errno %d\n", errno);
	}
}


void LoggerIn::log(const char* str) {
	log(str, strlen(str));
}


void LoggerIn::log(const std::string& str) {
	log(str.c_str(), str.length());
}

void LoggerIn::log(const boost::format& fmt) {
	log(fmt.str());
}


void LoggerIn::close() {
	if (pipeOut != -1) {
		::close(pipeOut);
		pipeOut = -1;
	}
}


void loopTick2Loggers(LoggerOut& log1, LoggerOut& log2) {
	bool log1Ok = true;
	bool log2Ok = true;

	while (log1Ok && log2Ok) {
		log1Ok = log1Ok && log1.tick(500); //smart. short-circuits ftw
		log2Ok = log2Ok && log2.tick(500);
	}

	log1.close();
	log2.close();
}
