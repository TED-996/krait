#pragma once
#include<vector>
#include <boost/regex.hpp>


class RegexList {
	std::vector<boost::regex> targets;

public:
	RegexList(std::vector<boost::regex> &targets) : targets(targets){
	}

	bool isMatch(std::string target);
	
	static RegexList fromFile(std::string filename);
};