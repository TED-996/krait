#pragma once
#include<vector>
#include <boost/python.hpp>
#include <boost/regex.hpp>


class RegexList
{
	std::vector<boost::regex> targets;

public:
	explicit RegexList(std::vector<boost::regex>& targets)
		: targets(targets) {
	}
	RegexList() {
	}

	bool isMatch(std::string target) const;

	static RegexList fromFile(std::string filename);

	static RegexList fromPythonObject(boost::python::object source);
};
