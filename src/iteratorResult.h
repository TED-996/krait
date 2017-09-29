#pragma once

#include <vector>
#include <boost/utility/string_ref.hpp>
#include "IResponseIterator.h"


class IteratorResult
{
private:
	std::vector<boost::string_ref> strIterated;
	std::vector<std::string> ownedStrings;

	size_t totalLength;
	size_t currentIdx;

	void exhaustIterator(IResponseIterator&& iterator);

public:
	IteratorResult(IResponseIterator&& iterator);
	IteratorResult(std::string fullString);
	IteratorResult(IteratorResult&& other) noexcept = default;
	IteratorResult& operator=(IteratorResult&& other) noexcept = default;

	size_t getTotalLength() const {
		return totalLength;
	}

	const IteratorResult& operator++();

	boost::string_ref operator*();
};
