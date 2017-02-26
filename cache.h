#pragma once
#include<string>
#include<boost/pool/object_pool.hpp>
#include<boost/filesystem.hpp>
#include<unordered_map>
#include<time>
#include"except.h"


template <typename T>
class FileCache {
private:
	struct CacheEntry{
		std::time_t time;
		const T* item;
	};
public:
	typedef const T* (*constructorFunction)(std::string filename, boost::object_pool<T>& pool);

private:
	boost::object_pool<T> pool;
	constructorFunction constructor;
	std::unordered_map<std::string, CacheEntry> cacheMap;

	const T* constructAddNew(std::string filename, std::time_t time){
		const T* result = constructor(filename, pool);
		cacheMap[filename] = CacheEntry {time, filename};
		return result;
	}

	const T* replaceWithNewer(std::string filename){
		const auto it = cacheMap.find(filename);

		if (!filesystem::exists(filename)){
			BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Not found: %1%", filename));
		}

		if (it != cacheMap.end() && pool.is_from(it->second.item)){
			pool.destroy(it->second.item);
			cacheMap.erase(it);
		}
		return constructAddNew(filename, filesystem::last_write_time(filename));
	}

public:
	FileCache(constructorFunction constructor)
		: constructor(constructor){
	}

	bool existsNewer(std::string filename, std::time_t time){
		if (!filesystem::exists(filename)){
			BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Not found: %1%", filename));
		}

		return (filesystem::last_write_time(filename) > time);
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
}