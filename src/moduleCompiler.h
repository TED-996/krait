#pragma once
#include <boost/filesystem/path.hpp>
#include "pymlFile.h"

class ModuleCompiler
{
	boost::filesystem::path siteRoot;
	
public:
	void compile(PymlFile& pymlFile, const std::string& destFilename, std::string&& cacheTag);
	std::string getCompiledModule(std::string moduleName);

	std::string escapeFilename(boost::string_ref filename) const;
	std::string unescapeFilename(boost::string_ref moduleName) const;
};
