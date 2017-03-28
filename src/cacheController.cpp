#include"cacheController.h"
#include"utils.h"
#include"formatHelper.h"

#include"dbg.h"

using namespace std;

CacheController::CacheController(string cachePrivateFilename, string cachePublicFilename, string cacheNoStoreFilename, string cacheLongTermFilename)
	:
		noStoreTargets(RegexList::fromFile(cacheNoStoreFilename)),
		privateTargets(RegexList::fromFile(cachePrivateFilename)),
		publicTargets(RegexList::fromFile(cachePublicFilename)),
		longTermTargets(RegexList::fromFile(cacheLongTermFilename)){
	maxAgeDefault = 300;
	maxAgeLongTerm = 864000;
}

CacheController::CachePragma CacheController::getCacheControl(string targetFilename, bool defaultIsStore){
	pair<string, bool> cacheKey = make_pair(targetFilename, defaultIsStore);

	const auto it = pragmaCache.find(cacheKey);
	if (it != pragmaCache.end()){
		return it->second;
	}

	CachePragma result;
	memzero(result);
	result.isStore = defaultIsStore;
	result.isCache = defaultIsStore;
	result.isRevalidate = true;
	if (noStoreTargets.isMatch(targetFilename)){
		result.isStore = false;
		result.isCache = false;
		result.isRevalidate = true;
	}
	if (privateTargets.isMatch(targetFilename)){
		result.isPrivate = true;
		result.isCache = true;
		result.isStore = true;
	}
	if (publicTargets.isMatch(targetFilename)){
		result.isPrivate = false;
		result.isCache = true;
		result.isStore = true;
	}

	//Modifiers (some assume stuff so just add them.)
	if (longTermTargets.isMatch(targetFilename)){
		result.isLongTerm = true;
		result.isCache = true;
		result.isStore = true;
	}

	pragmaCache[cacheKey] = result;

	return result;
}

string CacheController::getValueFromPragma(CacheController::CachePragma pragma){
	vector<string> result;
	if (!pragma.isCache){
		result.push_back("no-cache");
	}
	if (!pragma.isStore){
		result.push_back("no-store");
	}
	if (pragma.isPrivate){
		result.push_back("private");
	}
	if (pragma.isStore){
		if (pragma.isLongTerm){
			result.push_back(formatString("max-age=%d", maxAgeLongTerm));
		}
		else{
			result.push_back(formatString("max-age=%d", maxAgeDefault));			
		}
	}
	if (pragma.isRevalidate){
		result.push_back("must-revalidate");
	}

	if (result.size() == 0){
		return "";
	}
	else{
		string resultStr(result[0]);
		for (size_t i = 1; i < result.size(); i++){
			resultStr.append(", ", 2);
			resultStr.append(result[i]);
		}

		return resultStr;
	}
}
