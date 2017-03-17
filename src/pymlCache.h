#pragma once
#include "fileCache.h"
#include "pyml.h"

class PymlCache : public IPymlCache, public FileCache<PymlFile> {
public:
	PymlCache(FileCache<PymlFile>::constructorFunction constructor, FileCache<PymlFile>::cacheEventFunction onCacheEvent);
};
