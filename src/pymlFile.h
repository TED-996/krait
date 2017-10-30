#pragma once
#include <string>
#include <memory>
#include "IPymlParser.h"
#include "IPymlFile.h"
#include "pymlItems.h"


class PymlFile : public IPymlFile
{
private:
	std::unique_ptr<const IPymlItem> rootItem;
	std::unique_ptr<IPymlParser> parser;

public:
	PymlFile(std::string::iterator sourceStart,
	         std::string::iterator sourceEnd,
	         std::unique_ptr<IPymlParser>& parser);

	PymlFile(PymlFile&) = delete;
	PymlFile(PymlFile const&) = delete;

	bool isDynamic() const override;
	std::string runPyml() const override;

	const IPymlItem* getRootItem() const override {
		return rootItem.get();
	}

	bool canConvertToCode() const override {
		return getRootItem()->canConvertToCode();
	}
};
