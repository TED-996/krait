#pragma once
#include "IPymlFile.h"
#include "pyCompileModule.h"
#include "pyEmitModule.h"
#include "pythonModule.h"
#include "request.h"
#include "response.h"
#include <memory>

class PageResponseRenderer {
    CompiledPythonRunner& runner;

    PyEmitModule& emitModule;

public:
    PageResponseRenderer(CompiledPythonRunner& runner, PyEmitModule& emitModule);

    std::unique_ptr<Response> renderFromPyml(const IPymlFile& pymlSource, const Request& request);
    std::unique_ptr<Response> renderFromModuleName(boost::string_ref moduleName, const Request& request);
};
