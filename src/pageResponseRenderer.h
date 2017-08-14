#pragma once
#include <memory>
#include "request.h"
#include "IPymlFile.h"
#include "response.h"

class PageResponseRenderer
{
public:
	std::unique_ptr<Response> render(const IPymlFile& pymlSource, const Request& request);
};
