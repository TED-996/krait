#pragma once
#include <string>

class StringPiper {
    int readHead;
    int writeHead;

public:
    StringPiper();
    ~StringPiper();

    void closeRead();
    void closeWrite();
    void closePipes();

    void pipeWrite(std::string data);
    bool pipeAvailable();
    std::string pipeRead();
};
