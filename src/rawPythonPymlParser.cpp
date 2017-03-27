#include "rawPythonPymlParser.h"
#include "pythonWorker.h"

RawPythonPymlParser::RawPythonPymlParser()
	: pymlItem(""){
}

void RawPythonPymlParser::consume(std::string::iterator start, std::string::iterator end) {
	pymlItem = PymlItemPyExec(pythonPrepareStr(std::string(start, end)));
}

const IPymlItem *RawPythonPymlParser::getParsed() {
	return &pymlItem;
}
