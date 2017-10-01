#pragma once
#include <memory>
#include "request.h"
#include "response.h"
#include "pyEmitModule.h"


class PythonApiManager
{
	PyEmitModule emitModule;
public:
	void set(const Request& request, bool isWebsockets) const;
	std::unique_ptr<Response> getCustomResponse() const;
	bool isCustomResponse() const;
	void addHeaders(Response& response) const;
};
