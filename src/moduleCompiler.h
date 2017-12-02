#pragma once
#include "pymlFile.h"


class ModuleCompiler {
    static CodeAstItem getModuleHeader();
    static CodeAstItem getFunctionHeader();

public:
    void compile(const IPymlFile& pymlFile, const std::string& destFilename, std::string&& cacheTag);
};
