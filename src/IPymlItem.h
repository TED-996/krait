#pragma once

class IPymlItem
{
public:
	virtual std::string runPyml() const = 0;

	virtual bool isDynamic() const = 0;

	virtual const IPymlItem* getNext(const IPymlItem* last) const = 0;

	virtual const std::string* getEmbeddedString(std::string* storage) const = 0;

	virtual ~IPymlItem() = default;
};
