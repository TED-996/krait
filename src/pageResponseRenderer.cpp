#include "pageResponseRenderer.h"
#include "server.h"
#include "pythonModule.h"
#include "mvcPymlFile.h"
#include "pymlIterator.h"

#define DBG_DISABLE
#include "dbg.h"
#include "dbgStopwatch.h"


PageResponseRenderer::PageResponseRenderer() :
	emitModule() {
	emitModule.hideInstance();
}

std::unique_ptr<Response> PageResponseRenderer::renderFromPyml(const IPymlFile& pymlSource, const Request& request) {
	DbgStopwatch("Rendering response (from Pyml)");

	IteratorResult iterResult(PymlIterator(pymlSource.getRootItem()));
	
	return std::make_unique<Response>(200, std::move(iterResult), false);
}

std::unique_ptr<Response> PageResponseRenderer::renderFromModule(PythonModule& srcModule, const Request& request) {
	DbgStopwatch("Rendering response (from module)");
	
	emitModule.reset();
	emitModule.showInstance();

	try {
		srcModule.callGlobal("run");
	}
	catch (std::exception& ex) {
		emitModule.hideInstance();

		throw ex;
	}

	emitModule.hideInstance();

	return std::make_unique<Response>(200, IteratorResult(std::move(*emitModule.getIterator())), false);
}
