#pragma once
#include "pymlFile.h"

class ModuleCompiler
{
public:
	void compile(PymlFile& pymlFile, const std::string destFilename, std::string cacheTag);
};
