#pragma once
#include "pythonModule.h"

class CompiledModule : public PythonModule {
public:
    explicit CompiledModule(boost::python::object moduleObject) : PythonModule(moduleObject) {
    }

    boost::python::object run() {
        return callGlobal("run");
    }

    std::string getTag() {
        return getGlobalStr("tag");
    }

    void reload(); // TODO: move to PythonModule.
};
