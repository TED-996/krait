#include "pymlIterator.h"
#include "except.h"

//#define DBG_DISABLE
#include "dbg.h"

PymlIterator::PymlIterator(const IPymlItem* rootItem) {
    items.push(rootItem);

    lastValuePtr = rootItem->getEmbeddedString(&tmpStr);
    if (lastValuePtr == nullptr) {
        ++(*this);
    }
}

PymlIterator::PymlIterator(const PymlIterator& other)
        : items(other.items), tmpStr(other.tmpStr), lastValuePtr(other.lastValuePtr) {
    if (other.isTmpRef(boost::string_ref(lastValuePtr->data(), lastValuePtr->size()))) {
        this->lastValuePtr = &this->tmpStr;
    }
}

PymlIterator::PymlIterator(PymlIterator&& other) noexcept
        : items(std::move(other.items)), tmpStr(std::move(other.tmpStr)), lastValuePtr(other.lastValuePtr) {
    if (other.isTmpRef(boost::string_ref(lastValuePtr->data(), lastValuePtr->size()))) {
        this->lastValuePtr = &this->tmpStr;
    }
}

boost::string_ref PymlIterator::operator*() const {
    if (items.empty()) {
        return boost::string_ref();
    }

    return boost::string_ref(*lastValuePtr);
}

PymlIterator& PymlIterator::operator++() {
    if (items.empty()) {
        return *this;
    }

    const std::string* retStr = nullptr;
    while (retStr == nullptr && !items.empty()) {
        const IPymlItem* nextItem = items.top()->getNext(nullptr);
        while (nextItem == nullptr && !items.empty()) {
            const IPymlItem* lastItem = items.top();
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
        } else {
            retStr = nullptr;
        }
    }

    lastValuePtr = retStr;

    return *this;
}

bool PymlIterator::isTmpRef(const boost::string_ref& ref) const {
    bool result = ref.data() >= tmpStr.data() && ref.data() + ref.size() <= tmpStr.data() + tmpStr.size();
    if (result) {
        // DBG("------------!!!! this IS a tmpStr");
    }

    return result;
}
