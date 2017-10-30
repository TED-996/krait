#pragma once
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

	boost::filesystem::path siteRoot;
	Config& config;
	CacheController& cacheController;
	PymlCache& pymlCache;

	CompilerDispatcher compiler;
	CompiledPythonRunner compiledRunner;

	PythonApiManager apiManager;
	PageResponseRenderer renderer;

	std::unordered_map<std::string, std::string> contentTypeByExtension;

	PreResponseSource getSourceFromRequest(const Request& request) const;
	std::string getFilenameFromTarget(const std::string& target) const;
	static bool pathBlocked(const std::string& path);
	std::string expandFilename(const std::string& filename) const;

	const IPymlFile& getPymlFromCache(const std::string& filename) const;

	void addDefaultHeaders(Response& response, const std::string& filename, const Request& request,
		CacheController::CachePragma cachePragma, bool isDynamic);
	void addCacheHeaders(Response& response, const std::string& filename, CacheController::CachePragma pragma) const;
	
	std::string getContentType(const std::string& filename, bool isDynamic);
	void loadContentTypeList(const std::string& filename);

	std::unique_ptr<Response> buildResponseInternal(const Request& request, bool isWebsockets);

public:
	ResponseBuilder(const boost::filesystem::path& siteRoot, Config& config, CacheController& cacheController, PymlCache& pymlCache);

	std::unique_ptr<Response> buildResponse(Request& request);
	std::unique_ptr<Response> buildWebsocketsResponse(Request& request);
};
