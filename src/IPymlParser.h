#pragma once
#include <string>
#include <memory>
#include "IPymlItem.h"

class IPymlParser
{
public:
	virtual ~IPymlParser() = default;
	virtual void consume(std::string::iterator start, std::string::iterator end) = 0;
	virtual std::unique_ptr<const IPymlItem> getParsed() = 0;
};
