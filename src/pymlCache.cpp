#include <boost/filesystem.hpp>
#include "pymlCache.h"
#include"except.h"
#include"utils.h"
#include "dbg.h"


PymlCache::PymlCache(PymlCache::constructorFunction constructor,PymlCache::cacheEventFunction onCacheMiss)
	: constructor(constructor), onCacheMiss(onCacheMiss){
	frozen = false;
};

const IPymlFile* PymlCache::get(std::string filename){
	//DBG_FMT("PymlCache::get() on filename %1% (len %2%)", filename, filename.length());
	const auto it = cacheMap.find(filename);
	if (it == cacheMap.end() || existsNewer(filename, it->second.time)){
		return replaceWithNewer(filename);
	}
	else{
		return it->second.item;
	}
}

IPymlFile* PymlCache::constructAddNew(std::string filename, std::time_t time){
	CacheEntry resultEntry;
	memzero(resultEntry);

	PymlFile* result = constructor(filename, pool, resultEntry.tag);
	if (onCacheMiss != NULL){
		onCacheMiss(filename);
	}
	resultEntry.time = time;
	resultEntry.item = result;
	cacheMap[filename] = resultEntry;
	return result;
}

const IPymlFile* PymlCache::replaceWithNewer(std::string filename){
	const auto it = cacheMap.find(filename);

	//DBG_FMT("testing exists with filename %1%", filename);
	if (!boost::filesystem::exists(filename)){
		//DBG("except in replaceWithNewer");
		//DBG_FMT("reading anyway: %1%", readFromFile(filename));
		BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Not found: %1%", filename));
	}

	if (it != cacheMap.end() && pool.is_from(it->second.item)){
		pool.destroy(it->second.item);
		cacheMap.erase(it);
	}
	return constructAddNew(filename, boost::filesystem::last_write_time(filename));
}

bool PymlCache::existsNewer(std::string filename, std::time_t time){
	if (frozen){
		return true;
	}

	if (!boost::filesystem::exists(filename)){
		//DBG("except in existsNewer");
		BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Not found: %1%", filename));
	}

	return (boost::filesystem::last_write_time(filename) > time);
}

std::time_t PymlCache::getCacheTime(std::string filename){
	const auto it = cacheMap.find(filename);
	if (it == cacheMap.end() || existsNewer(filename, it->second.time)){
		replaceWithNewer(filename);
	}

	return cacheMap[filename].time;
}

bool PymlCache::checkCacheTag(std::string filename, std::string tag){
	const auto it = cacheMap.find(filename);
	if (it == cacheMap.end() || existsNewer(filename, it->second.time)){
		replaceWithNewer(filename);
	}

	return (strcmp(tag.c_str(), cacheMap[filename].tag) == 0);
}

std::string PymlCache::getCacheTag(std::string filename){
	const auto it = cacheMap.find(filename);
	if (it == cacheMap.end() || existsNewer(filename, it->second.time)){
		replaceWithNewer(filename);
	}

	return cacheMap[filename].tag;
}


