#pragma once
#include "IPymlParser.h"
#include "pymlItems.h"

class RawPythonPymlParser : public IPymlParser {
	IPymlCache& cache;
	PymlItemPyExec mainExec;
	PymlItemSeq rootSeq;
	PymlItemIf ctrlCondition;
	PymlItemSeq embedRootSeq;
	PymlItemPyExec embedSetupExec;
	PymlItemEmbed viewEmbed;
	PymlItemPyExec embedCleanupExec;
public:
	RawPythonPymlParser(IPymlCache& cache);
	void consume(std::string::iterator start, std::string::iterator end) override;

	const IPymlItem *getParsed() override;
};