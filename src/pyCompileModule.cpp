#include "pyCompileModule.h"
#include "pythonModule.h"
#include <boost/python.hpp>
#include <Python.h>


PyCompileModule* PyCompileModule::instance = nullptr;


BOOST_PYTHON_MODULE(_krait_compile) {
	boost::python::def("convert_filename", &PyCompileModule::convertFilename);
	boost::python::def("get_compiled_file", &PyCompileModule::getCompiledFile);
	boost::python::def("compiled_check_tag", &PyCompileModule::checkTag);
	boost::python::def("compiled_check_tag_or_reload", &PyCompileModule::checkTagOrReload);
	boost::python::def("compiled_run", &PyCompileModule::run);
	boost::python::def("get_module_reloaded_exception_type", &PyCompileModule::getExceptionType);
}


void PyCompileModule::initializeModule() {
	PyImport_AppendInittab("_krait_compile", &init_krait_compile);	
}

void PyCompileModule::postInitializeModule() {
	exceptionType = boost::python::object(
		boost::python::detail::new_reference(PyErr_NewException("_krait_compile.ModuleReloadedException", nullptr, nullptr)));
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
	return getInstanceOrThrow().dispatcher.getCompiler().escapeFilename(filename);
}

std::string PyCompileModule::getCompiledFile(boost::string_ref moduleName) {
	return getInstanceOrThrow().getCompiledFileInternal(moduleName);
}

std::string PyCompileModule::getCompiledFileInternal(boost::string_ref moduleName) {
	std::string sourcePath = dispatcher.getCompiler().unescapeFilename(moduleName);
	return dispatcher.compile(sourcePath);
}

bool PyCompileModule::checkTag(boost::string_ref moduleName, boost::string_ref tag) {
	return getInstanceOrThrow().dispatcher.getPymlCache().checkCacheTag(moduleName.to_string(), tag.to_string());
}

void PyCompileModule::checkTagOrReload(boost::string_ref moduleName, boost::string_ref tag) {
	PyCompileModule& instance = getInstanceOrThrow();
	if (!instance.dispatcher.getPymlCache().checkCacheTag(moduleName.to_string(), tag.to_string())) {
		instance.reload(moduleName);
	}
}

void PyCompileModule::reload(boost::string_ref moduleName) {
	PythonModule::reload(moduleName);

	PyErr_SetString(exceptionType.ptr(), "Module reloaded, please return from everything. Do not catch this.");
	boost::python::throw_error_already_set();
}

void PyCompileModule::run(boost::string_ref moduleName) {
	runner.run(moduleName);
}
