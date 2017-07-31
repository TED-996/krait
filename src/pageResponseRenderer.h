#pragma once
#include <boost/python/object.hpp>
#include "request.h"
#include "IPymlFile.h"
#include "IPymlCache.h"
#include "response.h"

class PageResponseRenderer
{
	IPymlCache& cache;
public:
	explicit PageResponseRenderer(IPymlCache& cache)
		: cache(cache) {
	}

	bool render(const IPymlFile& pymlSource, const Request& request, Response& destination);
	bool render(const boost::python::object& ctrlClass, const Request& request, Response& destination);
};
