#pragma once
#include "request.h"
#include "response.h"

class PythonApiManager
{
public:
	void set(const Request& request, bool isWebsockets);
	void getResponse(Response& response);
	void addHeaders(Response& response);
};
