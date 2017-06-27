#pragma once
#include <string>
#include "IPymlItem.h"

class IPymlParser
{
public:
	virtual void consume(std::string::iterator start, std::string::iterator end) = 0;
	virtual const IPymlItem* getParsed() = 0;
};
