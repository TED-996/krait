#include"cacheController.h"

#include"dbg.h"

using namespace std;

map<CacheController::CachePragma, string> CacheController::pragmaValueMappings {
	{CacheController::CachePragma::NoStore, "no-cache, no-store"},
	{CacheController::CachePragma::Private, "private"},
	{CacheController::CachePragma::Public, ""}
};

CacheController::CacheController(string cachePrivateFilename, string cachePublicFilename, string cacheDisableFilename)
	:
		noStoreTargets(RegexList::fromFile(cacheDisableFilename)),
		privateTargets(RegexList::fromFile(cachePrivateFilename)),
		publicTargets(RegexList::fromFile(cachePublicFilename)){	
}

CacheController::CachePragma CacheController::getCacheControl(string targetFilename){
	if (noStoreTargets.isMatch(targetFilename)){
		return CacheController::CachePragma::NoStore;
	}
	if (privateTargets.isMatch(targetFilename)){
		return CacheController::CachePragma::Private;
	}
	if (publicTargets.isMatch(targetFilename)){
		return CacheController::CachePragma::Public;
	}
	return CacheController::CachePragma::Default;
}

string CacheController::getValueFromPragma(CacheController::CachePragma pragma){
	string baseResult = pragmaValueMappings[pragma];
	return baseResult + (baseResult.empty() ? "must-revalidate" : ", must-revalidate");
}