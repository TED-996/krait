#pragma once
#include <memory>
#include "request.h"
#include "response.h"
#include "pyEmitModule.h"
#include "pyCompileModule.h"


class PythonApiManager
{
	PyEmitModule pyEmitModule;
	PyCompileModule pyCompileModule;
public:
	PythonApiManager(CompilerDispatcher& dispatcher, CompiledPythonRunner& runner);

	PyEmitModule& getPyEmitModule() {
		return pyEmitModule;
	}

	PyCompileModule& getPyCompileModule() {
		return pyCompileModule;
	}

	void set(const Request& request, bool isWebsockets) const;
	std::unique_ptr<Response> getCustomResponse() const;
	bool isCustomResponse() const;
	void addHeaders(Response& response) const;
};
