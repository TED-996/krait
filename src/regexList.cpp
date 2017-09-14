#include<fstream>
#include<boost/algorithm/string/trim.hpp>
#include"regexList.h"
#include"logger.h"
#include"except.h"

#define DBG_DISABLE
#include"dbg.h"

namespace bp = boost::python;

RegexList RegexList::fromFile(std::string filename) {
	std::ifstream fin(filename);
	std::string line;

	std::vector<boost::regex> resultVector;

	if (!fin) {
		resultVector.clear();
		Loggers::logErr(formatString("Cache targets config file %1% missing, add this file to further configure caching.", filename));
	}
	else {
		while (getline(fin, line)) {
			boost::trim_right_if(line, boost::is_any_of("\r\n"));
			resultVector.push_back(boost::regex(line));
		}
	}

	return RegexList(resultVector);
}

RegexList RegexList::fromPythonObject(boost::python::object source) {
	if (source.is_none()) {
		return RegexList();
	}

	long len = bp::len(source);
	
	std::vector<boost::regex> regexes;
	for (int i = 0; i < len; i++) {
		regexes.push_back(boost::regex(static_cast<std::string>(bp::extract<std::string>(bp::str(source[i])))));
	}
	return RegexList(regexes);
}

bool RegexList::isMatch(std::string target) const {
	boost::cmatch matchVariables;
	for (const auto& it: targets) {
		if (boost::regex_match(target.c_str(), matchVariables, it)) {
			return true;
		}
	}
	return false;
}

