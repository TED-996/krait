#pragma once

#include "pymlIterator.h"

class IteratorResult {
private:
	std::vector<std::string> strStorage;
	std::vector<const std::string*> strIterated;

	size_t totalLength;
	size_t currentIdx;

	void exhaustIterator(PymlIterator &iterator);

public:
	IteratorResult(PymlIterator iterator);
	IteratorResult(std::string fullString);

	size_t getTotalLength(){
		return totalLength;
	}

	const IteratorResult& operator++();

	const std::string* operator*();
};
