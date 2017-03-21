#include "rawPymlParser.h"


RawPymlParser::RawPymlParser()
	: pymlItem(""){
}

void RawPymlParser::consume(std::string::iterator start, std::string::iterator end) {
	pymlItem = PymlItemStr(std::string(start, end));
}

IPymlItem *RawPymlParser::getParsed() {
	return &pymlItem;
}
