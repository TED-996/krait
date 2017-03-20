#pragma once
#include<string>
#include<boost/pool/object_pool.hpp>
#include<unordered_map>
#include<ctime>
#include "IPymlCache.h"
#include "pymlFile.h"

class PymlCache : public IPymlCache {
private:
	struct CacheEntry{
		std::time_t time;
		PymlFile* item;
		char tag[33];
	};
public:
	typedef PymlFile* (*constructorFunction)(std::string filename, boost::object_pool<PymlFile>& pool, char* tagDest);
	typedef void (*cacheEventFunction)(std::string filename);

private:
	boost::object_pool<PymlFile> pool;
	constructorFunction constructor;
	cacheEventFunction onCacheMiss;
	bool frozen;

	std::unordered_map<std::string, CacheEntry> cacheMap;

	IPymlFile* constructAddNew(std::string filename, std::time_t time);
	const IPymlFile* replaceWithNewer(std::string filename);
	
public:
	PymlCache(PymlCache::constructorFunction constructor, PymlCache::cacheEventFunction onCacheMiss);
	const IPymlFile* get(std::string filename) override;

	bool existsNewer(std::string filename, std::time_t time);
	std::time_t getCacheTime(std::string filename);
	bool checkCacheTag(std::string filename, std::string tag);
	std::string getCacheTag(std::string filename);

	void freeze(){
		frozen = true;
	}

	void unfreeze(){
		frozen = false;
	}
};
