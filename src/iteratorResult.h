#pragma once

#include <memory>
#include "pymlIterator.h"
#include "valueOrPtr.h"

class IteratorResult
{
private:
	std::vector<ValueOrPtr<std::string>> strIterated;

	size_t totalLength;
	size_t currentIdx;

	void exhaustIterator(PymlIterator&& iterator);

public:
	IteratorResult(PymlIterator&& iterator);
	IteratorResult(std::string fullString);
	IteratorResult(IteratorResult&& other) noexcept;
	IteratorResult& operator=(IteratorResult&& other) noexcept;

	size_t getTotalLength() const {
		return totalLength;
	}

	const IteratorResult& operator++();

	const std::string* operator*();
};
