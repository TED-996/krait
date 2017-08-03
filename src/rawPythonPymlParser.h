#pragma once
#include "IPymlParser.h"
#include "pymlItems.h"

class RawPythonPymlParser : public IPymlParser
{
	IPymlCache& cache;
	std::unique_ptr<const PymlItemSeq> rootSeq;
public:
	RawPythonPymlParser(IPymlCache& cache);
	void consume(std::string::iterator start, std::string::iterator end) override;

	std::unique_ptr<const IPymlItem> getParsed() override;
};
