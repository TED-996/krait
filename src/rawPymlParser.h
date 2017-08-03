#pragma once
#include "IPymlParser.h"
#include "pymlItems.h"

class RawPymlParser : public IPymlParser
{
	std::unique_ptr<const PymlItemStr> pymlItem;

public:
	RawPymlParser();

	void consume(std::string::iterator start, std::string::iterator end);
	std::unique_ptr<const IPymlItem> getParsed() override;
};
