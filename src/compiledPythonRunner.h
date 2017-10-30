#pragma once
#include <string>
#include <boost/python/object.hpp>
#include <boost/utility/string_ref.hpp>

class CompiledPythonRunner
{
public:
	boost::python::object run(boost::string_ref name);
};
