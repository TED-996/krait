#pragma once
#include <string>
#include <iterator>
#include <memory>
#include "IPymlCache.h"
#include "IPymlParser.h"
#include "IPymlFile.h"
#include "pymlItems.h"


class PymlFile : public IPymlFile
{
private:
	const IPymlItem* rootItem;
	std::unique_ptr<IPymlParser> parser;

public:
	PymlFile(std::string::iterator sourceStart,
	         std::string::iterator sourceEnd,
	         std::unique_ptr<IPymlParser>& parser);

	PymlFile(PymlFile&) = delete;
	PymlFile(PymlFile const&) = delete;

	bool isDynamic() const;
	std::string runPyml() const;

	const IPymlItem* getRootItem() const {
		return (IPymlItem*)rootItem;
	}
};
