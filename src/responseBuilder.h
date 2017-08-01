﻿#pragma once
#include <boost/filesystem/path.hpp>
#include "request.h"
#include "cacheController.h"
#include "pymlCache.h"
#include "response.h"
#include "pageResponseRenderer.h"
#include "pythonApiManager.h"

class ResponseBuilder
{
private:
	struct PreResponseSource
	{
		const std::string symFilename;
		const boost::optional<std::string> sourceFilename;
		const boost::optional<const boost::python::object&> sourceObject;

		PreResponseSource(const std::string& symFilename, const std::string& sourceFilename)
			: symFilename(symFilename),
			  sourceFilename(sourceFilename),
			  sourceObject(boost::none) {
		}

		PreResponseSource(const std::string& symFilename, const boost::python::object& sourceObject)
			: symFilename(symFilename),
			  sourceFilename(boost::none),
			  sourceObject(std::move(boost::optional<const boost::python::object&>(sourceObject))) {
		}

		PreResponseSource(PreResponseSource&& other) noexcept
			: symFilename(std::move(other.symFilename)),
			sourceFilename(std::move(other.sourceFilename)),
			sourceObject(std::move(other.sourceObject)) {
		}

		PreResponseSource() 
			: symFilename(""),
			  sourceFilename(boost::none),
			  sourceObject(boost::none) {
		}
	};

	PythonApiManager apiManager;
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

	ResponseBuilder(const boost::filesystem::path& siteRoot, Config& config, CacheController& cacheController, PymlCache& pymlCache);

	Response buildResponse(Request& request);
	Response buildWebsocketsResponse(Request& request);
};
