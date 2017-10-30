#pragma once
#include "moduleCompiler.h"
#include "compilerDispatcher.h"
#include "compiledPythonRunner.h"

class PyCompileModule
{
	static PyCompileModule* instance;
	static boost::python::object exceptionType;

	CompilerDispatcher& dispatcher;
	CompiledPythonRunner& runner;

	static PyCompileModule& getInstanceOrThrow();

	std::string getCompiledFileInternal(boost::string_ref moduleName);
	void reload(boost::string_ref moduleName);
public:
	PyCompileModule(CompilerDispatcher& dispatcher, CompiledPythonRunner& runner);
	PyCompileModule(PyCompileModule&& other) noexcept;
	~PyCompileModule();

	static std::string convertFilename(boost::string_ref filename);
	static std::string getCompiledFile(boost::string_ref moduleName);
	static bool checkTag(boost::string_ref moduleName, boost::string_ref tag);
	static void checkTagOrReload(boost::string_ref moduleName, boost::string_ref tag);
	static void run(boost::string_ref moduleName);

	static boost::python::object getExceptionType() {
		return exceptionType;
	}

	static void initializeModule();
	static void postInitializeModule();
};
