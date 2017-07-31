#include "pythonApiManager.h"
#include "pythonModule.h"

void PythonApiManager::set(const Request& request, bool isDynamic, bool isWebsockets) {
	if (isDynamic || isWebsockets) {
		PythonModule::krait.setGlobalRequest("request", request);
	}
	if (isWebsockets) {
		PythonModule::websockets.run("request = WebsocketsRequest(krait.request)");
	}
}

bool PythonApiManager::getResponse(Response& response) {
}
