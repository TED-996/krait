#pragma once
#include "iteratorResult.h"
#include "pyEmitModule.h"
#include <boost/python/object.hpp>
#include <boost/utility/string_ref.hpp>
#include <string>

class CompiledPythonRenderer {
    PyEmitModule& emitModule;

public:
    void prepareForEmitIteration();

    boost::python::object run(boost::string_ref name);

    void discardEmitIteration();
    IteratorResult getEmitIteratorResult();
};
