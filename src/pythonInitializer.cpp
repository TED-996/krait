#include "pythonInitializer.h"
#include "pythonModule.h"
#include "logger.h"

PythonInitializer::PythonInitializer(const std::string& serverRoot) {
	try {
		PythonModule::initModules(serverRoot);
	}
	catch (pythonError& err) {
		Loggers::logErr(formatString("Error running init.py: %1%", err.what()));
		exit(1);
	}
}

PythonInitializer::~PythonInitializer() {
	PythonModule::finishPython();
}
