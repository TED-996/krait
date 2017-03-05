#pragma once
#include<string>
#include<boost/pool/object_pool.hpp>
#include<boost/filesystem.hpp>
#include<unordered_map>
#include<ctime>
#include"except.h"


template <typename T>
class FileCache {
private:
	struct CacheEntry{
		std::time_t time;
		T* item;
	};
public:
	typedef T* (*constructorFunction)(std::string filename, boost::object_pool<T>& pool);
	typedef void (*cacheEventFunction)(std::string filename);

private:
	boost::object_pool<T> pool;
	constructorFunction constructor;
	cacheEventFunction onCacheMiss;
	std::unordered_map<std::string, CacheEntry> cacheMap;

	T* constructAddNew(std::string filename, std::time_t time){
		T* result = constructor(filename, pool);
		if (onCacheMiss != NULL){
			onCacheMiss(filename);
		}
		cacheMap[filename] = CacheEntry {time, result};
		return result;
	}

	const T* replaceWithNewer(std::string filename){
		const auto it = cacheMap.find(filename);

		if (!boost::filesystem::exists(filename)){
			BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Not found: %1%", filename));
		}

		if (it != cacheMap.end() && pool.is_from(it->second.item)){
			pool.destroy(it->second.item);
			cacheMap.erase(it);
		}
		return constructAddNew(filename, boost::filesystem::last_write_time(filename));
	}

public:
	FileCache(constructorFunction constructor, cacheEventFunction onCacheMiss)
		: constructor(constructor), onCacheMiss(onCacheMiss){
	}

	bool existsNewer(std::string filename, std::time_t time){
		if (!boost::filesystem::exists(filename)){
			BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Not found: %1%", filename));
		}

		return (boost::filesystem::last_write_time(filename) > time);
	}

	/*
		Gets a file from cache; the file is updated if there is a newer version.
	*/
	const T* get(std::string filename){
		const auto it = cacheMap.find(filename);
		if (it == cacheMap.end() || existsNewer(filename, it->second.time)){
			return replaceWithNewer(filename);
		}
		else{
			return cacheMap[filename].item;
		}
	}

	std::time_t getCacheTime(std::string filename){
		const auto it = cacheMap.find(filename);
		if (it == cacheMap.end() || existsNewer(filename, it->second.time)){
			replaceWithNewer(filename);
		}

		return cacheMap[filename].time;
	}
};