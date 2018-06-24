#pragma once
#include "iteratorResult.h"
#include "pyEmitModule.h"
#include <boost/python/object.hpp>
#include <boost/utility/string_ref.hpp>
#include <string>

class CompiledPythonRenderer {
    PyEmitModule& emitModule;

public:
    explicit CompiledPythonRenderer(PyEmitModule& emitModule) : emitModule(emitModule) {
    }

    void prepareForEmitIteration() const;

    boost::python::object run(boost::string_ref name) const;

    void discardEmitIteration() const;
    IteratorResult getEmitIteratorResult() const;
};
