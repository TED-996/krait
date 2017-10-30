#pragma once
#include "moduleCompiler.h"
#include "pymlCache.h"

class CompilerDispatcher
{
	boost::filesystem::path compiledRoot;
	ModuleCompiler compiler;
	PymlCache& pymlCache;

	std::string getCompiledFilenameFromModuleName(const std::string& moduleName) const;
	std::string getCacheTag(boost::string_ref moduleName) const;
public:
	CompilerDispatcher(boost::filesystem::path siteRoot, PymlCache& pymlCache);

	const ModuleCompiler& getCompiler() const {
		return compiler;
	}

	PymlCache& getPymlCache() const {
		return pymlCache;
	}

	const boost::filesystem::path& getCompiledRoot() const {
		return compiledRoot;
	}

	// Returns the filename resulted from compiling the sourcePath file.
	std::string compile(const std::string& sourcePath);

	bool checkCacheTag(const std::string& moduleName) const;
	bool checkCacheTag(const std::string& moduleName, const std::string& computedTag) const;

	std::string getCompiledModuleName(boost::string_ref filename) const;
};
