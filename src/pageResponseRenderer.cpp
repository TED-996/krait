#include "pageResponseRenderer.h"
#include "server.h"
#include "pythonModule.h"
#include "mvcPymlFile.h"
#include "pymlIterator.h"

#define DBG_DISABLE
#include "dbg.h"
#include "dbgStopwatch.h"


PageResponseRenderer::PageResponseRenderer(CompiledPythonRunner& runner, PyEmitModule& emitModule)
	: runner(runner), emitModule(emitModule) {
	emitModule.hideInstance();
}

std::unique_ptr<Response> PageResponseRenderer::renderFromPyml(const IPymlFile& pymlSource, const Request& request) {
	DbgStopwatch("Rendering response (from Pyml)");

	IteratorResult iterResult(PymlIterator(pymlSource.getRootItem()));
	
	return std::make_unique<Response>(200, std::move(iterResult), false);
}

std::unique_ptr<Response> PageResponseRenderer::renderFromModuleName(boost::string_ref moduleName, const Request& request) {
	DbgStopwatch("Rendering response (from module)");
	
	emitModule.reset();
	emitModule.showInstance();

	try {
		boost::python::object responseResult = runner.run(moduleName);

		if (!responseResult.is_none()) {
			// Set this up to be caught by PythonApiManager
			PythonModule::krait().setGlobal("response", responseResult);
			// Get a dummy response that should be overwritten by PythonApiManager
			return std::make_unique<Response>(500, "<should not happen>", false);
		}
	}
	// The instance must be hidden even if an exception has ocurred.
	catch (std::exception& ex) {
		emitModule.hideInstance();

		throw;
	}

	emitModule.hideInstance();

	return std::make_unique<Response>(200, IteratorResult(std::move(*emitModule.getIterator())), false);
}
