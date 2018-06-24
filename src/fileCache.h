#pragma once
#include "except.h"
#include "utils.h"
#include <boost/filesystem.hpp>
#include <boost/pool/object_pool.hpp>
#include <ctime>
#include <string>
#include <unordered_map>


template<typename T>
class FileCache {
private:
    struct CacheEntry {
        std::time_t time;
        T* item;
        char tag[33];
    };

public:
    typedef T* (*constructorFunction)(std::string filename, boost::object_pool<T>& pool, char* tagDest);
    typedef void (*cacheEventFunction)(std::string filename);

private:
    boost::object_pool<T> pool;
    constructorFunction constructor;
    cacheEventFunction onCacheMiss;
    bool frozen;

    std::unordered_map<std::string, CacheEntry> cacheMap;

    T* constructAddNew(std::string filename, std::time_t time) {
        CacheEntry resultEntry;
        memzero(resultEntry);

        T* result = constructor(filename, pool, resultEntry.tag);
        if (onCacheMiss != nullptr) {
            onCacheMiss(filename);
        }
        resultEntry.time = time;
        resultEntry.item = result;
        cacheMap[filename] = resultEntry;
        return result;
    }

    const T* replaceWithNewer(std::string filename) {
        const auto it = cacheMap.find(filename);

        if (!boost::filesystem::exists(filename)) {
            BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Not found: %1%", filename));
        }

        if (it != cacheMap.end() && pool.is_from(it->second.item)) {
            pool.destroy(it->second.item);
            cacheMap.erase(it);
        }
        return constructAddNew(filename, boost::filesystem::last_write_time(filename));
    }

public:
    FileCache(constructorFunction constructor, cacheEventFunction onCacheMiss)
            : constructor(constructor), onCacheMiss(onCacheMiss) {
        frozen = false;
    }

    bool existsNewer(std::string filename, std::time_t time) {
        if (frozen) {
            return true;
        }

        if (!boost::filesystem::exists(filename)) {
            BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Not found: %1%", filename));
        }

        return (boost::filesystem::last_write_time(filename) > time);
    }

    /*
        Gets a file from cache; the file is updated if there is a newer version.
    */
    const T* get(std::string filename) {
        const auto it = cacheMap.find(filename);
        if (it == cacheMap.end() || existsNewer(filename, it->second.time)) {
            return replaceWithNewer(filename);
        } else {
            return it->second.item;
        }
    }

    std::time_t getCacheTime(std::string filename) {
        const auto it = cacheMap.find(filename);
        if (it == cacheMap.end() || existsNewer(filename, it->second.time)) {
            replaceWithNewer(filename);
        }

        return cacheMap[filename].time;
    }

    bool checkCacheTag(std::string filename, std::string tag) {
        const auto it = cacheMap.find(filename);
        if (it == cacheMap.end() || existsNewer(filename, it->second.time)) {
            replaceWithNewer(filename);
        }

        return (strcmp(tag.c_str(), cacheMap[filename].tag) == 0);
    }

    std::string getCacheTag(std::string filename) {
        const auto it = cacheMap.find(filename);
        if (it == cacheMap.end() || existsNewer(filename, it->second.time)) {
            replaceWithNewer(filename);
        }

        return cacheMap[filename].tag;
    }

    void freeze() {
        frozen = true;
    }

    void unfreeze() {
        frozen = false;
    }
};
