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

void RendererDispatcher::renderEmit(const IPymlFile& pymlFile) const {
    pymlRenderer.renderToEmit(pymlFile);
}

void RendererDispatcher::renderEmit(boost::python::object ctrlClass) const {
    pymlRenderer.renderToEmit(MvcPymlFile(ctrlClass, cache));
}
