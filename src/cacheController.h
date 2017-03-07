#include<string>
#include<map>
#include"regexList.h"


class CacheController {
public:
	enum CachePragma {Default, NoStore, Private, Public};
private:
	RegexList noStoreTargets;
	RegexList privateTargets;
	RegexList publicTargets;

	static std::map<CachePragma, std::string> pragmaValueMappings;
public:
	CacheController(std::string cachePrivateFilename, std::string cachePublicFilename, std::string cacheDisableFilename);

	CachePragma getCacheControl(std::string targetFilename);
	
	static std::string getValueFromPragma(CachePragma pragma);
};