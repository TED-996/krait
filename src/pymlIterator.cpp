#include"pymlIterator.h"
#include"except.h"

#define DBG_DISABLE
#include"dbg.h"

PymlIterator::PymlIterator(const IPymlItem* rootItem) {
	items.push(rootItem);

	lastValuePtr = rootItem->getEmbeddedString(&tmpStr);
	if (lastValuePtr == nullptr) {
		++(*this);
	}
}

PymlIterator::PymlIterator(const PymlIterator& other)
	: items(other.items), tmpStr(other.tmpStr), lastValuePtr(other.lastValuePtr) {
	if (other.isTmpStr(lastValuePtr)) {
		this->lastValuePtr = &this->tmpStr;
	}
}

PymlIterator::PymlIterator(PymlIterator&& other) noexcept
	: items(std::move(other.items)), tmpStr(std::move(other.tmpStr)), lastValuePtr(other.lastValuePtr){
	if (other.isTmpStr(lastValuePtr)) {
		this->lastValuePtr = &this->tmpStr;
	}
}

const std::string* PymlIterator::operator*() const {
	if (items.empty()) {
		return nullptr;
	}

	return lastValuePtr;
}

PymlIterator& PymlIterator::operator++() {
	if (items.empty()) {
		return *this;
	}

	const std::string* retStr = nullptr;
	while (retStr == nullptr && !items.empty()) {
		//Starting a new item.
		const IPymlItem* lastItem = nullptr;
		const IPymlItem* nextItem = items.top()->getNext(nullptr);
		while (nextItem == nullptr && !items.empty()) {
			lastItem = items.top();
			items.pop();
			if (!items.empty()) {
				nextItem = items.top()->getNext(lastItem);
			}
		}

		if (nextItem != nullptr) {
			items.push(nextItem);
		}

		if (!items.empty()) {
			retStr = items.top()->getEmbeddedString(&tmpStr);
		}
		else {
			retStr = nullptr;
		}
	}

	lastValuePtr = retStr;

	return *this;
}

bool PymlIterator::compareWith(std::string targetStr) {
	const char* targetPtr = targetStr.c_str();
	while (**this != nullptr) {
		if (memcmp((**this)->c_str(), targetPtr, (**this)->length()) != 0) {
			BOOST_THROW_EXCEPTION(serverError() << stringInfoFromFormat("Iterator comparison failed! Expected %1%, got %2%", targetPtr, ***this));
		}
		targetPtr += (**this)->length();
		++(*this);
	}

	if (targetPtr != targetStr.c_str() + targetStr.length()) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfoFromFormat("Iterator premature end! String %1% missing.", targetPtr));
	}
	return true;
}
