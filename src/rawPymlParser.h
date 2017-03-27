#pragma once
#include "IPymlParser.h"
#include "pymlItems.h"

class RawPymlParser : public IPymlParser {
	PymlItemStr pymlItem;

public:
	RawPymlParser();

	void consume(std::string::iterator start, std::string::iterator end);
	const IPymlItem* getParsed();
};
