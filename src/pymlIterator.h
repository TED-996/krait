#pragma once
#include<stack>
#include "pymlFile.h"

class PymlIterator
{
private:
	std::stack<const IPymlItem*> items;
	std::string tmpStr;
	const std::string* lastValuePtr;
public:
	PymlIterator(const IPymlItem* rootItem);
	PymlIterator(const PymlIterator& other);
	PymlIterator(PymlIterator&& other) noexcept;

	const std::string* operator*() const;

	PymlIterator& operator++();

	bool compareWith(std::string targetStr);
	
	bool isTmpStr(const std::string* strPtr) const {
		return strPtr == &tmpStr;
	}
};
