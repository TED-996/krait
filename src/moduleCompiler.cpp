#include "moduleCompiler.h"
#include <boost/filesystem/operations.hpp>
#include <fstream>

#define DBG_DISABLE
#include "dbg.h"


void ModuleCompiler::compile(const IPymlFile& pymlFile, const std::string& destFilename, std::string&& cacheTag) {
    const IPymlItem* rootItem = pymlFile.getRootItem();

    if (!rootItem->canConvertToCode()) {
        BOOST_THROW_EXCEPTION(compileError() << stringInfo("Tried compiling non-compilable PymlFile"));
    }

    std::unique_ptr<CodeAstItem> customHeader = nullptr;
    CodeAstItem codeAst = rootItem->getCodeAst();
    codeAst.addPassChildren("pass");

    // codeAst.optimize();

    std::ofstream outFile(destFilename);
    if (!outFile) {
        BOOST_THROW_EXCEPTION(serverError() << stringInfoFromFormat("Cannot open file %1% to compile", destFilename));
    }

    // Strip nulls from std::string.
    outFile << formatString(R"""(# [TAGS]: {"c_version": 1, "etag": "%1%"})""", cacheTag.c_str());
    outFile << "\n";

    static CodeAstItem moduleHeader = getModuleHeader();
    moduleHeader.getCodeToStream(0, outFile);

    if (customHeader != nullptr) {
        customHeader->addPassChildren("pass");
        customHeader->optimize();
        customHeader->getCodeToStream(0, outFile);
    }
    customHeader.reset();

    outFile << "\n\ndef run():\n";

    static CodeAstItem functionHeader = getFunctionHeader();
    functionHeader.getCodeToStream(1, outFile);

    codeAst.getCodeToStream(1, outFile);
}

CodeAstItem ModuleCompiler::getFunctionHeader() {
    CodeAstItem result;
    result.addChild(CodeAstItem("__emit__ = __internal._emit"));
    result.addChild(CodeAstItem("__emit_raw__ = __internal._emit_raw"));
    result.addChild(CodeAstItem("__run__ = __internal._compiled_run"));
    result.addChild(CodeAstItem("__to_module__ = __internal._compiled_convert_filename"));
    result.addChild(CodeAstItem("if hasattr(__loader__, 'check_tag_or_reload'): __loader__.check_tag_or_reload()"));
    result.addChild(CodeAstItem("ctrl = __mvc.curr_ctrl"));

    return result;
}

CodeAstItem ModuleCompiler::getModuleHeader() {
    CodeAstItem result;
    result.addChild(CodeAstItem("from __future__ import absolute_import"));
    result.addChild(CodeAstItem("import krait"));
    result.addChild(CodeAstItem("from krait import __internal"));
    result.addChild(CodeAstItem("from krait import mvc as __mvc"));

    return result;
}
