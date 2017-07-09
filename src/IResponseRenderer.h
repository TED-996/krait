#pragma once
#include "response.h"

class IResponseRenderer
{
public:
	virtual ~IResponseRenderer() = default;
	virtual bool render(const IPymlFile& source, std::string filename, Response* destination);
};
