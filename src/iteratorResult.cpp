#include "iteratorResult.h"

#define DBG_DISABLE
#include"dbg.h"

IteratorResult::IteratorResult(PymlIterator&& iterator) {
	exhaustIterator(std::move(iterator));
}

IteratorResult::IteratorResult(std::string fullString) {
	strIterated.emplace_back(std::make_unique<const std::string>(fullString));

	currentIdx = 0;
	totalLength = fullString.length();
}


IteratorResult::IteratorResult(IteratorResult&& other) noexcept
	: strIterated(std::move(other.strIterated)), totalLength(other.totalLength), currentIdx(other.currentIdx){
}

void IteratorResult::exhaustIterator(PymlIterator&& iterator) {
	totalLength = 0;
	while (*iterator != NULL) {
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
		return NULL;
	}
	return strIterated[currentIdx].get();
}
