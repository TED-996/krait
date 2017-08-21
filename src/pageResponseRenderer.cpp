#include "pageResponseRenderer.h"
#include "server.h"
#include "pythonModule.h"
#include "mvcPymlFile.h"

//#define DBG_DISABLE
#include "dbg.h"

std::unique_ptr<Response> PageResponseRenderer::render(const IPymlFile& pymlSource, const Request& request) {
	IteratorResult iterResult(PymlIterator(pymlSource.getRootItem()));
	
	return std::make_unique<Response>(200, std::move(iterResult), false);
}
