#pragma once
#include <boost/filesystem/path.hpp>
#include "request.h"
#include "cacheController.h"
#include "pymlCache.h"
#include "response.h"
#include "pageResponseRenderer.h"

class ResponseBuilder
{
private:
	struct PreResponseSource
	{
		std::string symFilename;
		boost::variant<std::string, const boost::python::object&> source;


		PreResponseSource(std::string symFilename, boost::variant<std::string, boost::python::object&> source)
			: symFilename(std::move(symFilename)),
			source(std::move(source)) {
		}
	};

	PageResponseRenderer renderer;

	Config& config;
	CacheController& cacheController;
	PymlCache& pymlCache;
	std::unordered_map<std::string, std::string> contentTypeByExtension;

	boost::filesystem::path siteRoot;

	PreResponseSource getSourceFromRequest(Request& request);
	std::string getFilenameFromTarget(const std::string& target) const;
	static bool pathBlocked(const std::string& path);
	std::string expandFilename(std::string filename) const;

	const IPymlFile& getPymlFromCache(std::string filename) const;

	void addDefaultHeaders(Response& response, std::string filename, const Request& request, CacheController::CachePragma cachePragma);
	void addCacheHeaders(Response& response, std::string filename, CacheController::CachePragma pragma) const;
	
	std::string getContentType(std::string filename);
	void loadContentTypeList(std::string filename);

	bool buildResponseInternal(Request& request, Response& response, bool isWebsockets);
public:

	ResponseBuilder(const boost::filesystem::path& siteRoot, Config& config, CacheController& cacheController, PymlCache& pymlCache)
		: renderer(pymlCache),
		  config(config),
		  cacheController(cacheController),
		  pymlCache(pymlCache),
		  siteRoot(siteRoot){
	}

	Response buildResponse(Request& request);
	Response buildWebsocketsResponse(Request& request);
};
