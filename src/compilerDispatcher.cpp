#include "compilerDispatcher.h"
#include "utils.h"

#define DBG_DISABLE
#include "dbg.h"


CompilerDispatcher::CompilerDispatcher(
    boost::filesystem::path siteRoot, PymlCache& pymlCache, SourceConverter& converter)
        : compiler(), pymlCache(pymlCache), converter(converter) {
    compiledRoot = siteRoot / ".compiled" / "_krait_compiled";
}

std::string CompilerDispatcher::compile(const std::string& sourcePath) {
    DBG_FMT("----CompilerDispatcher::compile(%1%)", sourcePath);

    const IPymlFile& pymlFile = pymlCache.get(sourcePath);
    std::string destFilename = getCompiledFilenameFromModuleName(converter.sourceToModuleName(sourcePath));

    compiler.compile(pymlFile, destFilename, pymlCache.getCacheTag(sourcePath));

    return destFilename;
}

std::string CompilerDispatcher::getCompiledFilenameFromModuleName(const std::string& moduleName) const {
    boost::string_ref moduleNameRef(moduleName);

    static boost::string_ref marker("_krait_compiled.");
    if (moduleNameRef.starts_with(marker)) {
        moduleNameRef.remove_prefix(marker.size());
    }

    return (compiledRoot / boost::filesystem::path(moduleNameRef.begin(), moduleNameRef.end())).string() + ".py";
}

bool CompilerDispatcher::checkCacheTag(const std::string& moduleName) const {
    return checkCacheTag(moduleName, getCacheTag(moduleName));
}

bool CompilerDispatcher::checkCacheTag(const std::string& moduleName, const std::string& computedTag) const {
    return pymlCache.checkCacheTag(converter.moduleNameToSource(moduleName), computedTag);
}

std::string CompilerDispatcher::getCacheTag(boost::string_ref moduleName) const {
    std::string sourceFilename = converter.moduleNameToSource(moduleName);
    return generateTagFromStat(sourceFilename);
}
