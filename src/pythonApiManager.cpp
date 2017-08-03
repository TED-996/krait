#include "pythonApiManager.h"
#include "pythonModule.h"

void PythonApiManager::set(const Request& request, bool isWebsockets) {
	PythonModule::krait.setGlobalRequest("request", request);
	
	if (isWebsockets) {
		PythonModule::websockets.run("request = WebsocketsRequest(krait.request)");
	}
}

void PythonApiManager::getResponse(Response& response) {
	if (!PythonModule::krait.checkIsNone("response")) {
		response = Response(PythonModule::krait.eval("str(response)"));
	}
}

void PythonApiManager::addHeaders(Response& response) {
	for (const auto& it : PythonModule::krait.getGlobalTupleList("extra_headers")) {
		response.addHeader(it.first, it.second);
	}
}
