#include "pageResponseRenderer.h"
#include "server.h"
#include "pythonModule.h"
#include "mvcPymlFile.h"

bool PageResponseRenderer::render(const IPymlFile& pymlSource, const Request& request, Response& destination) {
	IteratorResult iterResult(PymlIterator(pymlSource.getRootItem()));

	destination = Response(200, iterResult, false);

	return true;
}
