#include "pythonApiManager.h"
#include "pythonModule.h"

#define DBG_DISABLE
#include "dbg.h"


void PythonApiManager::set(const Request& request, bool isWebsockets) const {
	PythonModule::krait().setGlobalRequest("request", request);
	PythonModule::krait().setGlobalNone("response");
	PythonModule::krait().setGlobalEmptyList("extra_headers");
	PythonModule::krait().setGlobalNone("_content_type");
	PythonModule::cookie().setGlobalNone("_cookies_cpt");
	PythonModule::mvc().setGlobalNone("init_ctrl");
	PythonModule::mvc().setGlobalEmptyList("ctrl_stack");
	PythonModule::mvc().setGlobalNone("curr_ctrl");
	
	if (isWebsockets) {
		PythonModule::websockets().run("request = WebsocketsRequest(krait.request)");
		PythonModule::websockets().setGlobalNone("response");
	}
}

bool PythonApiManager::isCustomResponse() const {
	return !PythonModule::krait().checkIsNone("response");
}

std::unique_ptr<Response> PythonApiManager::getCustomResponse() const {
	DBG("PythonApiManager::getCustomResponse");
	return std::unique_ptr<Response>(PythonModule::krait().getGlobalResponsePtr("response"));
}

void PythonApiManager::addHeaders(Response& response) const {
	for (const auto& it : PythonModule::krait().getGlobalTupleList("extra_headers")) {
		response.addHeader(it.first, it.second);
	}
}
