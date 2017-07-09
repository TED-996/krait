#pragma once
#include "request.h"
#include "cacheController.h"
#include "pymlCache.h"
#include "response.h"

class ResponseBuilder
{
	CacheController& cacheController;
	PymlCache& pymlCache;
	std::unordered_map<std::string, std::string> contentTypeByExtension;


	void addDefaultHeaders(Response& response, std::string filename, Request& request);
	void addStandardCacheHeaders(Response& response, std::string filename, CacheController::CachePragma pragma);
	
	std::string getContentType(std::string filename);
	void loadContentTypeList(std::string filename);
public:
	Response buildResponse(Request& request);
};
