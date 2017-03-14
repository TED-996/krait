#pragma once
#include<iterator>
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
	PymlIterator operator++(int);

	bool compareWith(std::string other);

	bool isTmpStr(const std::string* strPtr){
		return strPtr == &tmpStr;
	}
};