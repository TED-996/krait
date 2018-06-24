#pragma once
#include "IPymlFile.h"
#include "iteratorResult.h"
#include "pyEmitModule.h"

class PymlRenderer {
    PyEmitModule& emitModule;

public:
    explicit PymlRenderer(PyEmitModule& emitModule) : emitModule(emitModule) {
    }
    PymlRenderer(PymlRenderer&& other) noexcept = default;
    PymlRenderer& operator=(PymlRenderer&& other) noexcept = default;

    IteratorResult renderToIterator(const IPymlFile& pymlFile) const;
    void renderToEmit(const IPymlFile& pymlFile) const;
};
