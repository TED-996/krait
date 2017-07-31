#pragma once
#include "request.h"
#include "response.h"

class PythonApiManager
{
public:
	void set(const Request& request, bool isDynamic, bool isWebsockets);
	bool getResponse(Response& response);
};
