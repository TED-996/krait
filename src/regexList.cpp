#include<fstream>
#include<boost/algorithm/string/trim.hpp>
#include"regexList.h"
#include"logger.h"
#include"except.h"

#define DBG_DISABLE
#include"dbg.h"

using namespace std;

RegexList RegexList::fromFile(string filename){
	ifstream fin(filename);
	string line;

	vector<boost::regex> resultVector;

	if (!fin){
		resultVector.clear();
		Loggers::logErr(formatString("Cache targets config file %1% missing, add this file to further configure caching.", filename));
	}
	else{
		while(getline(fin, line)){
			boost::trim_right_if(line, boost::is_any_of("\r\n"));
			//DBG_FMT("cache line is %1%", line);
			resultVector.push_back(boost::regex(line));
		}
	}

	return RegexList(resultVector);
}

bool RegexList::isMatch(string target){
	boost::cmatch matchVariables;
	for (const auto& it: targets){
		if (boost::regex_match(target.c_str(), matchVariables, it)){
			return true;
		}
	}
	return false;
}
