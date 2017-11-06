#pragma once
#include <boost/python/object.hpp>
#include <boost/utility/string_ref.hpp>
#include <string>

class CompiledPythonRunner {
public:
    boost::python::object run(boost::string_ref name);
};
