#pragma once
#include <boost/python/str.hpp>
#include <vector>

class PyEmitModule
{
	std::vector<boost::python::str> strings;
public:
	static void initialize();
	static void emit(boost::python::str value);
	static void emitRaw(boost::python::str value);
};
