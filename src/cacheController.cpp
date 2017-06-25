#include"cacheController.h"
#include"utils.h"
#include"formatHelper.h"
#include"pythonModule.h"

#define DBG_DISABLE
#include"dbg.h"

namespace bp = boost::python;


CacheController::CacheController(Config& config)
	:
	noStoreTargets(),
	privateTargets(),
	publicTargets(),
	longTermTargets() {
	maxAgeDefault = -1;
	maxAgeLongTerm = -1;

	loaded = false;
}

void CacheController::load() {
	this->noStoreTargets = RegexList::fromPythonObject(PythonModule::config.getGlobalVariable("cache_no_store"));
	this->privateTargets = RegexList::fromPythonObject(PythonModule::config.getGlobalVariable("cache_private"));
	this->publicTargets = RegexList::fromPythonObject(PythonModule::config.getGlobalVariable("cache_public"));
	this->longTermTargets = RegexList::fromPythonObject(PythonModule::config.getGlobalVariable("cache_long_term"));

	this->maxAgeDefault = bp::extract<int>(PythonModule::config.getGlobalVariable("cache_max_age_default"));
	this->maxAgeLongTerm = bp::extract<int>(PythonModule::config.getGlobalVariable("cache_max_age_long_term"));

	loaded = true;
}

CacheController::CachePragma CacheController::getCacheControl(std::string targetFilename, bool defaultIsStore) {
	if (!loaded) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Cache controller not loaded. Call load() first."));
	}

	std::pair<std::string, bool> cacheKey = make_pair(targetFilename, defaultIsStore);

	const auto it = pragmaCache.find(cacheKey);
	if (it != pragmaCache.end()) {
		return it->second;
	}

	CachePragma result;
	memzero(result);
	result.isStore = defaultIsStore;
	result.isCache = defaultIsStore;
	result.isRevalidate = true;
	if (noStoreTargets.isMatch(targetFilename)) {
		result.isStore = false;
		result.isCache = false;
		result.isRevalidate = true;
	}
	if (privateTargets.isMatch(targetFilename)) {
		result.isPrivate = true;
		result.isCache = true;
		result.isStore = true;
	}
	if (publicTargets.isMatch(targetFilename)) {
		result.isPrivate = false;
		result.isCache = true;
		result.isStore = true;
	}

	//Modifiers (some assume stuff so just add them.)
	if (longTermTargets.isMatch(targetFilename)) {
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
