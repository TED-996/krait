#pragma once
#include "compiledPythonRenderer.h"
#include "iteratorResult.h"
#include "pymlCache.h"
#include "pymlRenderer.h"
#include "pythonApiManager.h"
#include "response.h"
#include "responseSource.h"
#include <boost/optional.hpp>


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

    template<typename TReturn, typename TSrc>
    typedef TReturn (RendererDispatcher::*GetterFunction)(TSrc src);

    template<typename TReturn,
        GetterFunction<TReturn, const IPymlFile&> pymlGetter,
        GetterFunction<TReturn, boost::string_ref> moduleGetter,
        GetterFunction<TReturn, boost::python::object> mvcCtrlGetter>
    TReturn dispatch(ResponseSource& source) const {
        converter.extend(source);

        // If the pymlFile source exists, and is not dynamic, use it.
        if (source.hasPymlFile() && source.getPymlFile().isDynamic()) {
            return pymlGetter(source.getPymlFile());
        }

        // Next best thing: a compiled Python module
        // Either for a PymlFile on disk, or a MvcPymlFile.
        if (source.hasModuleName()
            && (source.hasPymlFile() && source.getPymlFile().canConvertToCode() || source.hasMvcController())) {
            return moduleGetter(source.getModuleName());
        }

        // If no module name, make a MvcPymlFile and interpret it.
        if (source.hasMvcController()) {
            return mvcCtrlGetter(source.getMvcController());
        }

        // Finally, interpret the file raw if you need.
        if (source.hasPymlFile()) {
            return pymlGetter(source.getPymlFile());
        }

        BOOST_THROW_EXCEPTION(serverError() << stringInfo("RendererDispatcher::dispatch with no source."));
    }

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
        // TODO this probably doesn't compile.
        dispatch<void, &renderEmit, &renderEmit, &renderEmit>(source);
    }

    IteratorResult get(const IPymlFile& pymlFile) const;
    IteratorResult get(boost::string_ref moduleName) const;
    IteratorResult get(boost::python::object ctrlClass) const;
    IteratorResult get(ResponseSource& source) const {
        return dispatch<IteratorResult, get, get, get>(source);
    }
};
