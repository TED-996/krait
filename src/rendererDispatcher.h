#pragma once
#include "compiledPythonRenderer.h"
#include "iteratorResult.h"
#include "pymlCache.h"
#include "pymlRenderer.h"
#include "responseSource.h"
#include "sourceConverter.h"


// TODO: split into RendererDispatcher and ResponseRendererDispatcher
// Removes circular dependency between PythonApiManager.PyCompileModule and RendererDispatcher.
class RendererDispatcher {
private:
    PymlCache& cache;
    PymlRenderer& pymlRenderer;
    CompiledPythonRenderer& compiledRenderer;
    SourceConverter& converter;

    // These assume that the best strategy has already been found.
    boost::python::object getFromModuleName(boost::string_ref moduleName) const;
    IteratorResult getFromPymlFile(const IPymlFile& pymlFile) const;
    IteratorResult getFromCtrlClass(boost::python::object ctrlClass) const;

public:
    class RenderStrategy {
    public:
        explicit RenderStrategy(const RendererDispatcher& dispatcher) : dispatcher(dispatcher) {
        }
        virtual ~RenderStrategy() = default;


    protected:
        const RendererDispatcher& dispatcher;

        virtual IteratorResult get(ResponseSource& source) = 0;
        virtual void emit(ResponseSource& source) = 0;

        IteratorResult getFromModuleName(boost::string_ref moduleName) const {
            return dispatcher.get(moduleName);
        }
        IteratorResult getFromPymlFile(const IPymlFile& pymlFile) const {
            return dispatcher.get(pymlFile);
        }
        IteratorResult getFromCtrlClass(boost::python::object ctrlClass) const {
            return dispatcher.get(ctrlClass);
        }
        void emitFromModuleName(boost::string_ref moduleName) const {
            dispatcher.renderEmit(moduleName);
        }
        void emitFromPymlFile(const IPymlFile& pymlFile) const {
            dispatcher.renderEmit(pymlFile);
        }
        void emitFromCtrlClass(boost::python::object ctrlClass) const {
            dispatcher.renderEmit(ctrlClass);
        }

        bool extendWithPymlFile(ResponseSource& source) const {
            return dispatcher.extendWithPymlFile(source);
        }
        bool extendWithModuleName(ResponseSource& source) const {
            return dispatcher.extendWithPymlFile(source);
        }

        friend class RendererDispatcher;
    };
    friend class RenderStrategy;

private:
    typedef std::unique_ptr<RenderStrategy> StrategyPtr;

    StrategyPtr dispatch(ResponseSource& source) const;

    bool extendWithPymlFile(ResponseSource& source) const;
    bool extendWithModuleName(ResponseSource& source) const;

public:
    RendererDispatcher(PymlCache& cache,
        PymlRenderer& pymlRenderer,
        CompiledPythonRenderer& compiledRenderer,
        SourceConverter& converter)
            : cache(cache), pymlRenderer(pymlRenderer), compiledRenderer(compiledRenderer), converter(converter) {
    }

    void renderEmit(const IPymlFile& pymlFile) const;
    void renderEmit(boost::string_ref moduleName) const;
    void renderEmit(boost::python::object ctrlClass) const;
    void renderEmit(ResponseSource& source) const {
        dispatch(source)->emit(source);
    }

    IteratorResult get(const IPymlFile& pymlFile) const;
    IteratorResult get(boost::string_ref moduleName) const;
    IteratorResult get(boost::python::object ctrlClass) const;
    IteratorResult get(ResponseSource& source) const {
        return dispatch(source)->get(source);
    }
};
