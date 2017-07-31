#include "rawPymlParser.h"
#include "except.h"


RawPymlParser::RawPymlParser()
	: pymlItem(std::make_unique<PymlItemStr>("")) {
}

void RawPymlParser::consume(std::string::iterator start, std::string::iterator end) {
	pymlItem = std::make_unique<PymlItemStr>(std::string(start, end));
}

std::unique_ptr<const IPymlItem> RawPymlParser::getParsed() {
	if (pymlItem == nullptr) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Pyml parser: root null (double consumption)"));
	}

	return std::move(pymlItem);
}
