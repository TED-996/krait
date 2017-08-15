#include "pythonApiManager.h"
#include "pythonModule.h"

void PythonApiManager::set(const Request& request, bool isWebsockets) const {
	PythonModule::krait.setGlobalRequest("request", request);
	
	if (isWebsockets) {
		PythonModule::websockets.run("request = WebsocketsRequest(krait.request)");
	}
}

bool PythonApiManager::isCustomResponse() const {
	return !PythonModule::krait.checkIsNone("response");
}

std::unique_ptr<Response> PythonApiManager::getCustomResponse() const {
	return std::make_unique<Response>(PythonModule::krait.eval("str(response)"));
}

void PythonApiManager::addHeaders(Response& response) const {
	for (const auto& it : PythonModule::krait.getGlobalTupleList("extra_headers")) {
		response.addHeader(it.first, it.second);
	}
}
