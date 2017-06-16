#include"pymlIterator.h"
#include"except.h"

#define DBG_DISABLE
#include"dbg.h"

PymlIterator::PymlIterator(const IPymlItem* rootItem) {
	items.push(rootItem);

	lastValuePtr = rootItem->getEmbeddedString(&tmpStr);
	if (lastValuePtr == NULL) {
		++(*this);
	}
}

PymlIterator::PymlIterator(const PymlIterator& other)
	: items(other.items), tmpStr(other.tmpStr), lastValuePtr(other.lastValuePtr) {
	if (other.lastValuePtr == &other.tmpStr) {
		this->lastValuePtr = &this->tmpStr;
	}
}

const std::string* PymlIterator::operator*() {
	if (items.empty()) {
		return NULL;
	}

	return lastValuePtr;
}

PymlIterator& PymlIterator::operator++() {
	if (items.empty()) {
		return *this;
	}

	const std::string* retStr = NULL;
	while (retStr == NULL && !items.empty()) {
		//Starting a new item.
		const IPymlItem* lastItem = NULL;
		const IPymlItem* nextItem = items.top()->getNext(NULL);
		while (nextItem == NULL && !items.empty()) {
			lastItem = items.top();
			items.pop();
			//DBG("pop");
			if (!items.empty()) {
				nextItem = items.top()->getNext(lastItem);
			}
		}

		if (nextItem != NULL) {
			items.push(nextItem);
			//DBG("push");
		}
		else {
			//DBG("null next...");
		}

		if (!items.empty()) {
			retStr = items.top()->getEmbeddedString(&tmpStr);
		}
		else {
			retStr = NULL;
		}
	}

	lastValuePtr = retStr;

	return *this;
}

bool PymlIterator::compareWith(std::string other) {
	const char* otherPtr = other.c_str();
	//DBG_FMT("**this: %p", **this);
	while (**this != NULL) {
		if (memcmp((**this)->c_str(), otherPtr, (**this)->length()) != 0) {
			BOOST_THROW_EXCEPTION(serverError() << stringInfoFromFormat("Iterator comparison failed! Expected %1%, got %2%", otherPtr, ***this));
		}
		otherPtr += (**this)->length();
		//DBG_FMT("len %d", (**this)->length());
		++(*this);
		//DBG("after ++");
	}
	//DBG("???");

	if (otherPtr != other.c_str() + other.length()) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfoFromFormat("Iterator premature end! String %1% missing.", otherPtr));
	}
	//DBG("Compare finished.");
	return true;
}
