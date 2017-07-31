#include "pageResponseRenderer.h"
#include "server.h"
#include "pythonModule.h"
#include "mvcPymlFile.h"

bool PageResponseRenderer::render(const IPymlFile& pymlSource, const Request& request, Response& destination) {
	bool isDynamic = pymlSource.isDynamic();

	IteratorResult iterResult(PymlIterator(pymlSource.getRootItem()));

	if (isDynamic && !PythonModule::krait.checkIsNone("response")) {
		destination.~Response();
		new(&destination) Response(PythonModule::krait.eval("str(response)"));
	}
	else {
		destination.~Response();
		new(&destination) Response(200, iterResult, false);
	}

	if (isDynamic) {
		for (const auto& it : PythonModule::krait.getGlobalTupleList("extra_headers")) {
			destination.addHeader(it.first, it.second);
		}
	}

	return true;
}

bool PageResponseRenderer::render(const boost::python::object& ctrlClass, const Request& request, Response& destination) {
	MvcPymlFile pymlFile = MvcPymlFile(ctrlClass, cache);
	return render(pymlFile, request, destination);
}
