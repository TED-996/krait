#pragma once
#include "IPymlFile.h"
#include "pyCompileModule.h"
#include "pyEmitModule.h"
#include "pythonModule.h"
#include "request.h"
#include "response.h"
#include <memory>

class PageResponseRenderer {
    CompiledPythonRenderer& renderer;

    PyEmitModule& emitModule;

public:
    PageResponseRenderer(CompiledPythonRenderer& renderer, PyEmitModule& emitModule);

    std::unique_ptr<Response> renderFromPyml(const IPymlFile& pymlSource, const Request& request);
    std::unique_ptr<Response> renderFromModuleName(boost::string_ref moduleName, const Request& request);
};
