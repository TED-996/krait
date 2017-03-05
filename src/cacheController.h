#include<vector>
#include <boost/regex.hpp>
#include<string>
#include<map>


class CacheController {
public:
	enum CachePragma {Default, NoCache, Private, Public};
private:
	std::vector<boost::regex> cachePrivateTargets;
	std::vector<boost::regex> cachePublicTargets;
	std::vector<boost::regex> cacheDisableTargets;

	static std::map<CachePragma, std::string> pragmaValueMappings;

	static void readRegexesIntoVector(std::string& filename, std::vector<boost::regex>& regexVector);
	static bool regexVectorIsAnyMatch(std::string& target, std::vector<boost::regex>& regexVector);
public:
	CacheController(std::string cachePrivateFilename, std::string cachePublicFilename, std::string cacheDisableFilename);

	CachePragma getCacheControl(std::string targetFilename);
	
	static std::string getValueFromPragma(CachePragma pragma);
};