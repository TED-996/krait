#pragma once
#include "IPymlItem.h"
#include <memory>
#include <string>

class IPymlParser {
public:
    virtual ~IPymlParser() = default;
    virtual void consume(std::string::iterator start, std::string::iterator end) = 0;
    virtual std::unique_ptr<const IPymlItem> getParsed() = 0;
};
