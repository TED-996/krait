#pragma once
#include <memory>
#include "request.h"
#include "IPymlFile.h"
#include "response.h"
#include "pythonModule.h"
#include "pyEmitModule.h"
#include "pyCompileModule.h"

class PageResponseRenderer
{
	CompiledPythonRunner& runner;

	PyEmitModule& emitModule;
public:
	PageResponseRenderer(CompiledPythonRunner& runner, PyEmitModule& emitModule);

	std::unique_ptr<Response> renderFromPyml(const IPymlFile& pymlSource, const Request& request);
	std::unique_ptr<Response> renderFromModuleName(boost::string_ref moduleName, const Request& request);
};
