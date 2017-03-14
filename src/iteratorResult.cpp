#include "iteratorResult.h"

#include"dbg.h"

using namespace std;

IteratorResult::IteratorResult(PymlIterator iterator){
	exhaustIterator(iterator);
}

IteratorResult::IteratorResult(std::string fullString) {
	strStorage.push_back(fullString);
	strIterated.push_back(&(strStorage[0]));

	currentIdx = 0;
	totalLength = fullString.length();
}


void IteratorResult::exhaustIterator(PymlIterator &iterator) {
	totalLength = 0;
	while(*iterator != NULL){
		if (iterator.isTmpStr(*iterator)){
			strStorage.push_back(string(**iterator));
			strIterated.push_back(&strStorage[strStorage.size() - 1]);
		}
		else{
			strIterated.push_back(*iterator);
		}
		totalLength += (*iterator)->length();
		DBG_FMT("exhausting iterator: total length %d", totalLength);

		++iterator;
	}
	DBG_FMT("exhausting iterator: total length %d", totalLength);
	currentIdx = 0;
}

const IteratorResult& IteratorResult::operator++() {
	if (currentIdx < strIterated.size()) {
		currentIdx++;
	}
	return *this;
}

const std::string *IteratorResult::operator*() {
	DBG_FMT("iterated len: %d, idx %d", strIterated.size(), currentIdx);

	if (currentIdx >= strIterated.size()){
		return NULL;
	}
	return strIterated[currentIdx];
}



