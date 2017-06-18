#include "iteratorResult.h"

#define DBG_DISABLE
#include"dbg.h"

IteratorResult::IteratorResult(PymlIterator iterator) {
	exhaustIterator(iterator);
}

IteratorResult::IteratorResult(std::string fullString) {
	strIterated.push_back(ValueOrPtr<std::string>(fullString));

	currentIdx = 0;
	totalLength = fullString.length();
}


void IteratorResult::exhaustIterator(PymlIterator& iterator) {
	totalLength = 0;
	while (*iterator != NULL) {
		if (iterator.isTmpStr(*iterator)) {
			strIterated.push_back(ValueOrPtr<std::string>(**iterator));
			//DBG_FMT("added to storage: %1%", *strIterated[strIterated.size() - 1].get());
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
