#pragma once
#include <memory>
#include "request.h"
#include "IPymlFile.h"
#include "response.h"
#include "pythonModule.h"
#include "pyEmitModule.h"

class PageResponseRenderer
{
	PyEmitModule emitModule;
public:
	PageResponseRenderer();

	std::unique_ptr<Response> renderFromPyml(const IPymlFile& pymlSource, const Request& request);
	std::unique_ptr<Response> renderFromModule(PythonModule& srcModule, const Request& request);
};
