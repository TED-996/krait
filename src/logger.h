#pragma once
#include <boost/format.hpp>
#include <fstream>
#include <unistd.h>

// TOOD: include Logger in this file

class LoggerOut {
    int pipeIn;
    std::ofstream outfile;

public:
    LoggerOut();
    LoggerOut(int pipeIn);
    LoggerOut(int pipeIn, const std::string& filename);

    ~LoggerOut() {
        close();
    }

    bool tick(int timeoutMs);

    void close();
};


class LoggerIn {
    int pipeOut;

public:
    LoggerIn() : pipeOut(dup(1)) {
    }

    explicit LoggerIn(int pipeOut);

    LoggerIn(LoggerIn& other) : pipeOut(dup(other.pipeOut)) {
    }

    ~LoggerIn() {
        close();
    }

    void log(const char* buffer, size_t size);
    void log(const char* str);
    void log(const std::string& str);
    void log(const boost::format& fmt);

    void close();
};


class Loggers {
public:
    static LoggerIn infoLogger;
    static LoggerIn errLogger;

    static void setLoggers(int infoPipe, int errPipe);
    static void setInfoLogger(int infoPipe);
    static void setErrLogger(int errPipe);

    static void logInfo(const std::string& message);
    static void logErr(const std::string& message);

    static LoggerIn& getInfoLogger() {
        return infoLogger;
    }

    static LoggerIn& getErrLogger() {
        return errLogger;
    }

private:
    Loggers() {
    }
};


void loopTick2Loggers(LoggerOut& log1, LoggerOut& log2);
