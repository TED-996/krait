#pragma once
#include "pyCompileModule.h"
#include "pyEmitModule.h"
#include "request.h"
#include "response.h"
#include <memory>


class PythonApiManager {
    PyEmitModule& pyEmitModule;
    PyCompileModule& pyCompileModule;

    void set(const Request& request, bool isWebsockets) const;

public:
    PythonApiManager(PyEmitModule& pyEmitModule, PyCompileModule& pyCompileModule)
            : pyEmitModule(pyEmitModule), pyCompileModule(pyCompileModule) {
    }

    PyEmitModule& getPyEmitModule() {
        return pyEmitModule;
    }

    PyCompileModule& getPyCompileModule() {
        return pyCompileModule;
    }

    void setRegularRequest(const Request& request) {
        set(request, false);
    }

    void setWebsocketsRequest(const Request& request) {
        set(request, true);
    }

    static void setCustomResponse(const boost::python::object& response);

    std::unique_ptr<Response> getCustomResponse() const;
    bool isCustomResponse() const;
    void addHeaders(Response& response) const;
};
