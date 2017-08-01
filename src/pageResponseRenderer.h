#pragma once
#include "request.h"
#include "IPymlFile.h"
#include "response.h"

class PageResponseRenderer
{
public:
	bool render(const IPymlFile& pymlSource, const Request& request, Response& destination);
};
