#include "rendererDispatcher.h"
#include "mvcPymlFile.h"
#include "pythonModule.h"


IteratorResult RendererDispatcher::getFromPymlFile(const IPymlFile& pymlFile) const {
    return pymlRenderer.renderToIterator(pymlFile);
}

boost::python::object RendererDispatcher::getFromModuleName(boost::string_ref moduleName) const {
    return compiledRenderer.run(moduleName);
}

IteratorResult RendererDispatcher::getFromCtrlClass(boost::python::object ctrlClass) const {
    return pymlRenderer.renderToIterator(MvcPymlFile(ctrlClass, cache));
}

IteratorResult RendererDispatcher::get(const IPymlFile& pymlFile) const {
    return getFromPymlFile(pymlFile);
}

IteratorResult RendererDispatcher::get(boost::string_ref moduleName) const {
    compiledRenderer.prepareForEmitIteration();
    boost::python::object response = getFromModuleName(moduleName);
    if (!response.is_none()) {
        compiledRenderer.discardEmitIteration();
        // A bit of a circular dependency, but such is life.
        PythonApiManager::setCustomResponse(response);
        return IteratorResult("<h1>THIS IS A BUG</h1>");
    }
    return compiledRenderer.getEmitIteratorResult();
}

IteratorResult RendererDispatcher::get(boost::python::object ctrlClass) const {
    return getFromCtrlClass(ctrlClass);
}

void RendererDispatcher::renderEmit(boost::string_ref moduleName) const {
    compiledRenderer.run(moduleName);
}

bool RendererDispatcher::extendWithPymlFile(ResponseSource& source) const {
    if (source.hasPymlFile()) {
        return true;
    }
    converter.extend(source);
    return source.hasPymlFile();
}

bool RendererDispatcher::extendWithModuleName(ResponseSource& source) const {
    if (source.hasModuleName()) {
        return true;
    }
    converter.extend(source);
    return source.hasModuleName();
}


class CompiledRenderStrategy : public RendererDispatcher::RenderStrategy {
public:
    explicit CompiledRenderStrategy(const RendererDispatcher& dispatcher) : RenderStrategy(dispatcher) {
    }

protected:
    IteratorResult get(ResponseSource& source) override {
        if (!extendWithModuleName(source)) {
            BOOST_THROW_EXCEPTION(
                serverError() << stringInfo("CompiledRenderStrategy dispatched without a module name."));
        }
        return getFromModuleName(source.getModuleName());
    }
    void emit(ResponseSource& source) override {
        if (!source.hasModuleName()) {
            BOOST_THROW_EXCEPTION(
                serverError() << stringInfo("CompiledRenderStrategy dispatched without a module name."));
        }
        emitFromModuleName(source.getModuleName());
    }
};

class InterpretedRenderStrategy : public RendererDispatcher::RenderStrategy {
public:
    explicit InterpretedRenderStrategy(const RendererDispatcher& dispatcher) : RenderStrategy(dispatcher) {
    }

protected:
    IteratorResult get(ResponseSource& source) override {
        if (!extendWithPymlFile(source)) {
            BOOST_THROW_EXCEPTION(
                serverError() << stringInfo("InterpretedRenderStrategy dispatched without a Pyml fle."));
        }
        return getFromPymlFile(source.getPymlFile());
    }
    void emit(ResponseSource& source) override {
        if (!extendWithPymlFile(source)) {
            BOOST_THROW_EXCEPTION(
                serverError() << stringInfo("InterpretedRenderStrategy dispatched without a Pyml fle."));
        }
        return emitFromPymlFile(source.getPymlFile());
    }
};


RendererDispatcher::StrategyPtr RendererDispatcher::dispatch(ResponseSource& source) const {
    converter.extend(source);

    // If the pymlFile source exists, and is not dynamic, use it.
    if (source.hasPymlFile() && source.getPymlFile().isDynamic()) {
        return std::make_unique<InterpretedRenderStrategy>(*this);
    }

    // Next best thing: a compiled Python module
    // Either for a PymlFile on disk, or a MvcPymlFile.
    if (source.hasModuleName()
        && (source.hasPymlFile() && source.getPymlFile().canConvertToCode() || source.hasMvcController())) {
        return std::make_unique<CompiledRenderStrategy>(*this);
    }

    // If no module name, make a MvcPymlFile and interpret it.
    if (source.hasMvcController()) {
        return std::make_unique<InterpretedRenderStrategy>(*this);
    }

    // Finally, interpret the file raw if you need.
    if (source.hasPymlFile()) {
        return std::make_unique<InterpretedRenderStrategy>(*this);
    }

    BOOST_THROW_EXCEPTION(serverError() << stringInfo("RendererDispatcher::dispatch with no source."));
}

void RendererDispatcher::renderEmit(const IPymlFile& pymlFile) const {
    pymlRenderer.renderToEmit(pymlFile);
}

void RendererDispatcher::renderEmit(boost::python::object ctrlClass) const {
    pymlRenderer.renderToEmit(MvcPymlFile(ctrlClass, cache));
}
