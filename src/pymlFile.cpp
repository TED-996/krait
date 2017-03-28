#include"dbg.h"

#include "pymlFile.h"

using namespace std;


PymlFile::PymlFile(std::string::iterator sourceStart,
                   std::string::iterator sourceEnd,
                   std::unique_ptr<IPymlParser>& parser)
		: parser(std::move(parser)){
	//DBG("preConsume");
	//DBG_FMT("parser: %1%", parser.get());
	this->parser->consume(sourceStart, sourceEnd);
	rootItem = this->parser->getParsed();
}

std::string PymlFile::runPyml() const {
	if (rootItem == NULL){
		return "";
	}
	return rootItem->runPyml();
}


bool PymlFile::isDynamic() const {
	if (rootItem == NULL){
		return false;
	}
	return rootItem->isDynamic();
}



