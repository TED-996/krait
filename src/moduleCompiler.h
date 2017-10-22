#pragma once
#include <boost/filesystem/path.hpp>
#include "pymlFile.h"

class ModuleCompiler
{
	boost::filesystem::path siteRoot;

	static CodeAstItem getModuleHeader();
	static CodeAstItem getFunctionHeader();
public:
	explicit ModuleCompiler(boost::filesystem::path siteRoot);

	void compile(const IPymlFile& pymlFile, const std::string& destFilename, std::string&& cacheTag);

	std::string escapeFilename(boost::string_ref filename) const;
	std::string unescapeFilename(boost::string_ref moduleName) const;
};
