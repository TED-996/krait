#pragma once
#include <boost/filesystem/path.hpp>
#include "request.h"
#include "cacheController.h"
#include "pymlCache.h"
#include "response.h"

class ResponseBuilder
{
	Config& config;
	CacheController& cacheController;
	PymlCache& pymlCache;
	std::unordered_map<std::string, std::string> contentTypeByExtension;

	boost::filesystem::path siteRoot;

	std::string getSourceFromRequest(Request& request);
	std::string getFilenameFromTarget(const std::string& target) const;
	static bool pathBlocked(const std::string& path);
	std::string expandFilename(std::string filename) const;

	const IPymlFile& getPymlFromCache(std::string filename) const;

	void addDefaultHeaders(Response& response, std::string filename, const Request& request, CacheController::CachePragma cachePragma);
	void addCacheHeaders(Response& response, std::string filename, CacheController::CachePragma pragma) const;
	
	std::string getContentType(std::string filename);
	void loadContentTypeList(std::string filename);
public:
	Response buildResponse(Request& request);
};
