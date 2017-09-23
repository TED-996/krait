#include <boost/filesystem.hpp>
#include "cacheController.h"
#include "utils.h"
#include "formatHelper.h"
#include "pythonModule.h"

#define DBG_DISABLE
#include "dbg.h"

namespace bp = boost::python;


CacheController::CacheController(Config& config, std::string filenameRoot) :
	noStoreTargets(config.getNoStoreTargets()),
	privateTargets(config.getPrivateTargets()),
	publicTargets(config.getPublicTargets()),
	longTermTargets(config.getLongTermTargets()),
	filenameRoot(filenameRoot){
	maxAgeDefault = config.getMaxAgeDefault();
	maxAgeLongTerm = config.getMaxAgeLongTerm();
}

CacheController::CachePragma CacheController::getCacheControl(std::string targetFilename, bool defaultIsStore) {
	std::string relFilename = boost::filesystem::relative(targetFilename, filenameRoot).string();

	std::pair<std::string, bool> cacheKey = std::make_pair(relFilename, defaultIsStore);

	const auto it = pragmaCache.find(cacheKey);
	if (it != pragmaCache.end()) {
		return it->second;
	}

	CachePragma result;
	memzero(result);
	result.isStore = defaultIsStore;
	result.isCache = defaultIsStore;
	result.isRevalidate = true;
	if (noStoreTargets.isMatch(relFilename)) {
		result.isStore = false;
		result.isCache = false;
		result.isRevalidate = true;
	}
	if (privateTargets.isMatch(relFilename)) {
		result.isPrivate = true;
		result.isCache = true;
		result.isStore = true;
	}
	if (publicTargets.isMatch(relFilename)) {
		result.isPrivate = false;
		result.isCache = true;
		result.isStore = true;
	}

	//Modifiers (some assume stuff so just add them.)
	if (longTermTargets.isMatch(relFilename)) {
		result.isLongTerm = true;
		result.isCache = true;
		result.isStore = true;
	}

	pragmaCache[cacheKey] = result;

	return result;
}

std::string CacheController::getValueFromPragma(CacheController::CachePragma pragma) {
	std::vector<std::string> result;
	if (!pragma.isCache) {
		result.push_back("no-cache");
	}
	if (!pragma.isStore) {
		result.push_back("no-store");
	}
	if (pragma.isPrivate) {
		result.push_back("private");
	}
	if (pragma.isStore) {
		if (pragma.isLongTerm) {
			result.push_back(formatString("max-age=%d", maxAgeLongTerm));
		}
		else {
			result.push_back(formatString("max-age=%d", maxAgeDefault));
		}
	}
	if (pragma.isRevalidate) {
		result.push_back("must-revalidate");
	}

	if (result.size() == 0) {
		return "";
	}
	else {
		std::string resultStr(result[0]);
		for (size_t i = 1; i < result.size(); i++) {
			resultStr.append(", ", 2);
			resultStr.append(result[i]);
		}

		return resultStr;
	}
}
