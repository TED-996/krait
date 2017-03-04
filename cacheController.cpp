#include<fstream>
#include<boost/algorithm/string/trim.hpp>
#include"cacheController.h"
#include"logger.h"
#include"except.h"

#include"dbg.h"

using namespace std;

map<CacheController::CachePragma, string> CacheController::pragmaValueMappings {
	{CacheController::CachePragma::NoCache, "no-cache"},
	{CacheController::CachePragma::Private, "private"},
	{CacheController::CachePragma::Public, "public"}
};

CacheController::CacheController(string cachePrivateFilename, string cachePublicFilename, string cacheDisableFilename){
	readRegexesIntoVector(cachePrivateFilename, cachePrivateTargets);
	readRegexesIntoVector(cachePublicFilename, cachePublicTargets);
	readRegexesIntoVector(cacheDisableFilename, cacheDisableTargets);
}

void CacheController::readRegexesIntoVector(string& filename, vector<boost::regex>& regexVector){
	ifstream fin(filename);
	string line;

	if (!fin){
		regexVector.clear();
		Loggers::logErr(formatString("Cache targets config file %1% missing, add this file to further configure caching.", filename));
	}
	else{
		while(getline(fin, line)){
			boost::trim_right_if(line, boost::is_any_of("\r\n"));
			DBG_FMT("cache line is %1%", line);
			regexVector.push_back(boost::regex(line));
		}
	}
}

CacheController::CachePragma CacheController::getCacheControl(string targetFilename){
	// DBG_FMT("!!trying to get cache control for %1%", targetFilename);
	if (regexVectorIsAnyMatch(targetFilename, cacheDisableTargets)){
		return CacheController::CachePragma::NoCache;
	}
	if (regexVectorIsAnyMatch(targetFilename, cachePrivateTargets)){
		return CacheController::CachePragma::Private;
	}
	if (regexVectorIsAnyMatch(targetFilename, cachePublicTargets)){
		return CacheController::CachePragma::Public;
	}
	return CacheController::CachePragma::Default;
}

bool CacheController::regexVectorIsAnyMatch(string& target, vector<boost::regex>& regexVector){
	boost::cmatch matchVariables;
	for (const auto& it: regexVector){
		if (boost::regex_match(target.c_str(), matchVariables, it)){
			// DBG("matched private!");
			return true;
		}
	}
	return false;
}


string CacheController::getValueFromPragma(CacheController::CachePragma pragma){
	return pragmaValueMappings[pragma];
}