#include "pyCompileModule.h"
#include "pythonModule.h"
#include <Python.h>
#include <boost/python.hpp>


PyCompileModule* PyCompileModule::instance = nullptr;
boost::python::object PyCompileModule::exceptionType;


PyCompileModule::PyCompileModule(
    CompilerDispatcher& dispatcher, RendererDispatcher& renderer, SourceConverter& converter)
        : dispatcher(dispatcher), renderer(renderer), converter(converter) {
    if (instance != nullptr) {
        BOOST_THROW_EXCEPTION(serverError() << stringInfo("Multiple PyCompileModule instances"));
    }
    instance = this;
}

PyCompileModule::PyCompileModule(PyCompileModule&& other) noexcept
        : dispatcher(other.dispatcher), renderer(other.renderer), converter(other.converter) {
    if (instance == &other) {
        instance = this;
    }
}

PyCompileModule::~PyCompileModule() {
    if (instance == this) {
        instance = nullptr;
    }
}

BOOST_PYTHON_MODULE(_krait_compile) {
    boost::python::def("convert_filename", &PyCompileModule::convertFilename);
    boost::python::def("get_compiled_file", &PyCompileModule::getCompiledFile);
    boost::python::def("check_tag", &PyCompileModule::checkTag);
    boost::python::def("check_tag_or_reload", &PyCompileModule::checkTagOrReload);
    boost::python::def("run", &PyCompileModule::run);
    boost::python::def("get_module_reloaded_exception_type", &PyCompileModule::getExceptionType);
}


void PyCompileModule::initializeModule() {
    PyImport_AppendInittab("_krait_compile", &init_krait_compile);
}

void PyCompileModule::postInitializeModule() {
    char exceptionName[] = "_krait_compile.ModuleReloadedException";
    exceptionType = boost::python::object(
        boost::python::detail::new_reference(PyErr_NewException(exceptionName, nullptr, nullptr)));
}


PyCompileModule& PyCompileModule::getInstanceOrThrow() {
    PyCompileModule* instanceLocal = instance;
    if (instanceLocal == nullptr) {
        PyErr_SetString(PyExc_ValueError, "Krait Compile not available, server not running.");
        boost::python::throw_error_already_set();
    }
    return *instanceLocal;
}

std::string PyCompileModule::convertFilename(boost::string_ref filename) {
    return getInstanceOrThrow().converter.sourceToModuleName(filename);
}

std::string PyCompileModule::getCompiledFile(boost::string_ref moduleName) {
    return getInstanceOrThrow().getCompiledFileInternal(moduleName);
}

std::string PyCompileModule::getCompiledFileInternal(boost::string_ref moduleName) {
    std::string sourcePath = converter.moduleNameToSource(moduleName);
    return dispatcher.compile(sourcePath);
}

bool PyCompileModule::checkTag(boost::string_ref moduleName, boost::string_ref tag) {
    return getInstanceOrThrow().dispatcher.checkCacheTag(moduleName.to_string(), tag.to_string());
}

void PyCompileModule::checkTagOrReload(boost::string_ref moduleName, boost::string_ref tag) {
    PyCompileModule& instance = getInstanceOrThrow();
    if (!instance.dispatcher.checkCacheTag(moduleName.to_string(), tag.to_string())) {
        instance.reload(moduleName);
    }
}

void PyCompileModule::reload(boost::string_ref moduleName) {
    PythonModule::reload(moduleName.to_string());

    PyErr_SetString(exceptionType.ptr(), "Module reloaded, please return from everything. Do not catch this.");
    boost::python::throw_error_already_set();
}

void PyCompileModule::run(boost::string_ref moduleName) {
    getInstanceOrThrow().renderer.renderEmit(moduleName);
}
