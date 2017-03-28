#pragma once
#include "IPymlParser.h"
#include "pymlItems.h"

class RawPythonPymlParser : public IPymlParser {
	PymlItemPyExec pymlItem;
public:
	RawPythonPymlParser();
	void consume(std::string::iterator start, std::string::iterator end) override;

	const IPymlItem *getParsed() override;
};