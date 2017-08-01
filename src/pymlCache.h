#pragma once
#include<string>
#include<unordered_map>
#include<ctime>
#include <functional>
#include "IPymlCache.h"
#include "pymlFile.h"

class PymlCache : public IPymlCache
{
public:
	struct CacheTag
	{
		char data[32];


		CacheTag();
		explicit CacheTag(std::string data);
		CacheTag(const CacheTag& source);
		CacheTag(CacheTag&& source) noexcept;

		void setTag(std::string data);

		bool operator== (const CacheTag& other) const;
		bool operator!= (const CacheTag& other) const;
		explicit operator std::string() const;
	};
private:
	struct CacheEntry
	{
		std::time_t time;
		std::unique_ptr<PymlFile> item;
		CacheTag tag;


		CacheEntry(std::time_t time, std::unique_ptr<PymlFile>&& pymlFile, CacheTag& tag)
			: time(time),
			  item(std::move(pymlFile)),
			  tag(std::move(tag)) {
		}
		CacheEntry() = default;
		CacheEntry(CacheEntry&& entry) = default;
	};


public:
	typedef std::function<std::unique_ptr<PymlFile>(std::string filename, CacheTag& tag)> constructorFunction;
	typedef std::function<void(std::string filename)> cacheEventFunction;

private:
	constructorFunction constructor;
	cacheEventFunction onCacheMiss;
	bool frozen;

	std::unordered_map<std::string, CacheEntry> cacheMap;

	IPymlFile& constructAddNew(std::string filename, std::time_t time);
	const IPymlFile& replaceWithNewer(std::string filename);

public:
	PymlCache(PymlCache::constructorFunction constructor, PymlCache::cacheEventFunction onCacheMiss);
	const IPymlFile& get(std::string filename) override;

	bool existsNewer(std::string filename, std::time_t time);
	std::time_t getCacheTime(std::string filename);
	bool checkCacheTag(std::string filename, std::string tag);
	std::string getCacheTag(std::string filename);

	void freeze() {
		frozen = true;
	}

	void unfreeze() {
		frozen = false;
	}
};
