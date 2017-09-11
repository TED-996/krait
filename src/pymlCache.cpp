#include <boost/filesystem.hpp>
#include "pymlCache.h"
#include"except.h"

#define DBG_DISABLE
#include "dbg.h"


PymlCache::PymlCache(PymlCache::constructorFunction constructor, PymlCache::cacheEventFunction onCacheMiss)
	: constructor(constructor), onCacheMiss(onCacheMiss) {
	frozen = false;
};

const IPymlFile& PymlCache::get(std::string filename) {
	const auto it = cacheMap.find(filename);
	if (it == cacheMap.end() || existsNewer(filename, it->second.time)) {
		return replaceWithNewer(filename);
	}
	else {
		return *it->second.item.get();
	}
}

IPymlFile& PymlCache::constructAddNew(std::string filename, std::time_t time) {
	CacheTag tag;

	std::unique_ptr<PymlFile> result = constructor(filename, tag);
	if (onCacheMiss != nullptr) {
		onCacheMiss(filename);
	}

	cacheMap.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(filename),
		std::forward_as_tuple(time, std::move(result), tag)
	);
	return *cacheMap[filename].item;
}

const IPymlFile& PymlCache::replaceWithNewer(std::string filename) {
	const auto it = cacheMap.find(filename);

	if (!boost::filesystem::exists(filename)) {
		BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Not found: %1%", filename));
	}

	if (it != cacheMap.end()) {
		cacheMap.erase(it);
	}
	return constructAddNew(filename, boost::filesystem::last_write_time(filename));
}

bool PymlCache::existsNewer(std::string filename, std::time_t time) {
	if (frozen) {
		return true;
	}

	if (!boost::filesystem::exists(filename)) {
		BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Not found: %1%", filename));
	}

	return (boost::filesystem::last_write_time(filename) > time);
}

std::time_t PymlCache::getCacheTime(std::string filename) {
	const auto it = cacheMap.find(filename);
	if (it == cacheMap.end() || existsNewer(filename, it->second.time)) {
		replaceWithNewer(filename);
	}

	return cacheMap[filename].time;
}

bool PymlCache::checkCacheTag(std::string filename, std::string tag) {
	const auto it = cacheMap.find(filename);
	if (it == cacheMap.end() || existsNewer(filename, it->second.time)) {
		replaceWithNewer(filename);
	}

	return cacheMap[filename].tag == CacheTag(tag);
}

std::string PymlCache::getCacheTag(std::string filename) {
	const auto it = cacheMap.find(filename);
	if (it == cacheMap.end() || existsNewer(filename, it->second.time)) {
		replaceWithNewer(filename);
	}

	return (std::string)cacheMap[filename].tag;
}


PymlCache::CacheTag::CacheTag() {
	std::memset(this->data, 0, sizeof(this->data));
}

PymlCache::CacheTag::CacheTag(std::string data) {
	this->setTag(data);
}

void PymlCache::CacheTag::setTag(std::string data) {
	if (data.size() * sizeof(std::string::value_type) > sizeof(this->data)) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Tag too large."));
	}

	std::memset(this->data, 0, sizeof(this->data));
	std::memcpy(this->data, data.c_str(),
		std::min(data.size() * sizeof(std::string::value_type), sizeof(this->data)));
}

PymlCache::CacheTag::CacheTag(const CacheTag& source) {
	std::memcpy(this->data, source.data, sizeof(this->data));
}

PymlCache::CacheTag::CacheTag(CacheTag&& source) noexcept {
	std::memcpy(this->data, source.data, sizeof(this->data));
}

bool PymlCache::CacheTag::operator==(const CacheTag& other) const {
	return std::memcmp(this->data, other.data, sizeof(this->data)) == 0;
}

bool PymlCache::CacheTag::operator!=(const CacheTag& other) const {
	return !(this->operator==(other));
}

PymlCache::CacheTag::operator std::string() const {
	return std::string(this->data, sizeof(this->data));
}
