#include "iteratorResult.h"

#define DBG_DISABLE
#include "dbg.h"

IteratorResult::IteratorResult(IResponseIterator&& iterator) {
    exhaustIterator(std::move(iterator));
}

IteratorResult::IteratorResult(std::string&& fullString) {
    if (fullString.size() != 0) {
        ownedStrings.emplace_back(std::move(fullString));
        strIterated.emplace_back(ownedStrings.back());
    }

    currentIdx = 0;
    totalLength = fullString.length();
}

void IteratorResult::exhaustIterator(IResponseIterator&& iterator) {
    totalLength = 0;
    while ((*iterator).data() != nullptr) {
        if (iterator.isTmpRef(*iterator)) {
            ownedStrings.emplace_back((*iterator).data(), (*iterator).length());
            strIterated.push_back(boost::string_ref(ownedStrings.back()));
        } else {
            strIterated.push_back(*iterator);
        }
        totalLength += (*iterator).length();

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

boost::string_ref IteratorResult::operator*() {
    if (currentIdx >= strIterated.size()) {
        return boost::string_ref();
    }
    return strIterated[currentIdx];
}
