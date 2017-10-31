﻿#include "compilerDispatcher.h"
#include "utils.h"

//#define DBG_DISABLE
#include "dbg.h"


CompilerDispatcher::CompilerDispatcher(boost::filesystem::path siteRoot, PymlCache& pymlCache)
	: compiledRoot(siteRoot / ".compiled" / "_krait_compiled"),
	compiler(siteRoot),
	pymlCache(pymlCache) {
}

std::string CompilerDispatcher::compile(const std::string& sourcePath) {
	DBG_FMT("----CompilerDispatcher::compile(%1%)", sourcePath);

	const IPymlFile& pymlFile = pymlCache.get(sourcePath);
	std::string destFilename = getCompiledFilenameFromModuleName(compiler.escapeFilename(sourcePath));

	compiler.compile(pymlFile, destFilename, getCacheTag(sourcePath));

	return destFilename;
}


std::string CompilerDispatcher::getCompiledFilenameFromModuleName(const std::string& moduleName) const {
	return (compiledRoot / moduleName).string();
}

std::string CompilerDispatcher::getCompiledModuleName(boost::string_ref filename) const {
	return compiler.escapeFilename(filename);
}

bool CompilerDispatcher::checkCacheTag(const std::string& moduleName) const {
	return checkCacheTag(moduleName, getCacheTag(moduleName));
}

bool CompilerDispatcher::checkCacheTag(const std::string& moduleName, const std::string& computedTag) const {
	return pymlCache.checkCacheTag(compiler.unescapeFilename(moduleName), computedTag);
}

std::string CompilerDispatcher::getCacheTag(boost::string_ref moduleName) const {
	std::string sourceFilename = compiler.unescapeFilename(moduleName);
	return generateTagFromStat(sourceFilename);
}
