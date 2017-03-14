#pragma once
#include<stack>
#include"pyml.h"

class PymlIterator {
private:
	std::stack<const PymlItem*> items;
	std::string tmpStr;
	const std::string* lastValuePtr;
public:
	PymlIterator(const PymlItem* rootItem);
	PymlIterator(const PymlIterator& other);

	const std::string* operator*();

	PymlIterator& operator++();

	bool compareWith(std::string other);

	bool isTmpStr(const std::string* strPtr){
		return strPtr == &tmpStr;
	}
};