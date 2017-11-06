#include "pymlFile.h"

//#define DBG_DISABLE
#include "dbg.h"
#include "dbgStopwatch.h"


PymlFile::PymlFile(
    std::string::iterator sourceStart, std::string::iterator sourceEnd, std::unique_ptr<IPymlParser>& parser)
        : parser(std::move(parser)) {
    DbgStopwatch("PymlFile parsing");

    this->parser->consume(sourceStart, sourceEnd);
    rootItem = this->parser->getParsed();
}

std::string PymlFile::runPyml() const {
    if (rootItem == nullptr) {
        return "";
    }
    return rootItem->runPyml();
}


bool PymlFile::isDynamic() const {
    if (rootItem == nullptr) {
        return false;
    }
    return rootItem->isDynamic();
}
