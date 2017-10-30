#pragma once
#include "IPymlItem.h"

class IPymlFile
{
public:
	virtual ~IPymlFile() = default;
	virtual bool isDynamic() const = 0;
	virtual std::string runPyml() const = 0;
	virtual const IPymlItem* getRootItem() const = 0;

	virtual bool canConvertToCode() const = 0;
};
