#include "iteratorResult.h"

#define DBG_DISABLE
#include"dbg.h"

IteratorResult::IteratorResult(PymlIterator&& iterator) {
	exhaustIterator(std::move(iterator));
}

IteratorResult::IteratorResult(std::string fullString) {
	strIterated.emplace_back(fullString);

	currentIdx = 0;
	totalLength = fullString.length();
}


IteratorResult::IteratorResult(IteratorResult&& other) noexcept
	: strIterated(std::move(other.strIterated)), totalLength(other.totalLength), currentIdx(other.currentIdx){
}

IteratorResult& IteratorResult::operator=(IteratorResult&& other) noexcept {
	if (this == &other)
		return *this;
	strIterated = std::move(other.strIterated);
	totalLength = other.totalLength;
	currentIdx = other.currentIdx;
	return *this;
}

void IteratorResult::exhaustIterator(PymlIterator&& iterator) {
	totalLength = 0;
	while (*iterator != nullptr) {
		if (iterator.isTmpStr(*iterator)) {
			strIterated.push_back(ValueOrPtr<std::string>(**iterator));
		}
		else {
			strIterated.push_back(ValueOrPtr<std::string>(*iterator));
		}
		totalLength += (*iterator)->length();

		++iterator;
	}
	currentIdx = 0;
}

const IteratorResult& IteratorResult::operator++() {
	if (currentIdx < strIterated.size()) {
		currentIdx++;
	}
	return *this;
}

const std::string* IteratorResult::operator*() {
	if (currentIdx >= strIterated.size()) {
		return nullptr;
	}
	return strIterated[currentIdx].get();
}
