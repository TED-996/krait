#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "responseBuilder.h"
#include "pythonModule.h"
#include "utils.h"
#include "path.h"


//#define DBG_DISABLE
#include "dbg.h"
#include "logger.h"
#include "mvcPymlFile.h"

namespace bf = boost::filesystem;
namespace ba = boost::algorithm;


ResponseBuilder::ResponseBuilder(const boost::filesystem::path& siteRoot, Config& config, CacheController& cacheController, PymlCache& pymlCache)
	: config(config),
	  cacheController(cacheController),
	  pymlCache(pymlCache),
	  siteRoot(siteRoot) {
	loadContentTypeList((getShareRoot() / "globals" / "mime.types").string());
}


bool ResponseBuilder::buildResponseInternal(Request& request, Response& response, bool isWebsockets) {
	bool isHead = (request.getVerb() == HttpVerb::HEAD);
	bool okResponse;
	std::string symFilename = "<unavailable>";

	try {
		PreResponseSource source = std::move(getSourceFromRequest(request));
		symFilename = source.symFilename;
		bool isDynamic;
		
		std::string etag;
		if (request.headerExists("if-none-match")) {
			etag = request.getHeader("if-none-match").get();
			etag = etag.substr(1, etag.length() - 2);
		}

		boost::optional<const IPymlFile&> pymlFile = boost::none;
		std::unique_ptr<MvcPymlFile> mvcFileStorage = nullptr;

		if (source.sourceFilename) {
			pymlFile = getPymlFromCache(source.sourceFilename.get());
			isDynamic = pymlFile->isDynamic();
		}
		else if(source.sourceObject) {
			isDynamic = true;
			mvcFileStorage = std::make_unique<MvcPymlFile>(source.sourceObject.get(), pymlCache);
			pymlFile = *mvcFileStorage.get();
		}
		else {
			BOOST_THROW_EXCEPTION(serverError() << stringInfo("ResponseBuilder::getSourceFromRequest returned empty PreResponseSource."));
		}
		
		CacheController::CachePragma cachePragma = cacheController.getCacheControl(source.symFilename, !isDynamic);

		if (cachePragma.isStore && pymlCache.checkCacheTag(source.symFilename, etag)) {
			response = Response(304, "", false);
		}
		else {
			if (isDynamic) {
				apiManager.set(request, isWebsockets);
			}

			renderer.render(pymlFile.get(), request, response);

			if (isDynamic) {
				apiManager.getResponse(response);
				apiManager.addHeaders(response);
			}
		}

		addDefaultHeaders(response, source.symFilename, request, cachePragma);
		okResponse = true;
	}
	catch(notFoundError) {
		Loggers::logInfo(formatString("Not found building %1%", symFilename)); //TODO: encapsulate some info in NotFoundError
		response = Response(404, "<html><body><h1>404 Not Found</h1></body></html>", true);
		okResponse = false;
	}

	if (isHead) {
		response.setBody("", false);
	}

	return okResponse;
}

Response ResponseBuilder::buildResponse(Request& request) {
	Response response(500, "", true);

	buildResponseInternal(request, response, false);

	return response;
}

Response ResponseBuilder::buildWebsocketsResponse(Request& request) {
	Response response(500, "", true);
	request.setRouteVerb(RouteVerb::WEBSOCKET);
	
	buildResponseInternal(request, response, true);

	return response;
}

std::string replaceParams(std::string target, std::map<std::string, std::string> params);

ResponseBuilder::PreResponseSource ResponseBuilder::getSourceFromRequest(Request& request) {
	std::map<std::string, std::string> params;
	const Route& route = Route::getRouteMatch(config.getRoutes(), request.getRouteVerb(), request.getUrl(), params);
	
	std::string symFilename;

	if (!route.isMvcRoute()) {
		std::string targetReplaced = replaceParams(route.getTarget(request.getUrl()), params);
		symFilename = expandFilename(getFilenameFromTarget(targetReplaced));
	}
	else {
		symFilename = getFilenameFromTarget(request.getUrl());
	}

	if (pathBlocked(symFilename)) {
		Loggers::logInfo(formatString("Tried accessing path %1%, BLOCKED.", symFilename));
		BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Error: File not found: %1%", symFilename));
	}

	if (!route.isMvcRoute()) {
		std::string targetReplaced = replaceParams(route.getTarget(request.getUrl()), params);
		symFilename = expandFilename(getFilenameFromTarget(targetReplaced));
		return PreResponseSource(symFilename, symFilename);
	}
	else {
		symFilename = getFilenameFromTarget(request.getUrl());
		return PreResponseSource(symFilename, route.getCtrlClass());
	}
}


std::string ResponseBuilder::getFilenameFromTarget(const std::string& target) const {
	if (target[0] == '/') {
		return (siteRoot / target.substr(1)).string();
	}
	else {
		return (siteRoot / target).string();
	}
}


std::string ResponseBuilder::expandFilename(std::string filename) const {
	if (bf::is_directory(filename)) {
		//DBG("Converting to directory automatically");
		filename = (bf::path(filename) / "index").string(); //Not index.html; taking care below about it
	}

	if (!bf::exists(filename)) {
		if (bf::exists(filename + ".html")) {
			//DBG("Adding .html automatically");
			filename += ".html";
		}
		else if (bf::exists(filename + ".htm")) {
			//DBG("Adding .htm automatically");
			filename += ".htm";
		}
		else if (bf::exists(filename + ".pyml")) {
			//DBG("Adding .pyml automatically");
			filename += ".pyml";
		}
		else if (bf::exists(filename + ".py")) {
			filename += ".py";
		}

		else if (bf::path(filename).extension() == ".html" &&
			bf::exists(bf::change_extension(filename, ".htm"))) {
			filename = bf::change_extension(filename, "htm").string();
			//DBG("Changing extension to .htm");
		}
		else if (bf::path(filename).extension() == ".htm" &&
			bf::exists(bf::change_extension(filename, ".html"))) {
			filename = bf::change_extension(filename, "html").string();
			//DBG("Changing extension to .html");
		}
	}
	return filename;
}

const IPymlFile& ResponseBuilder::getPymlFromCache(std::string filename) const {
	return pymlCache.get(filename);
}
 
void ResponseBuilder::addDefaultHeaders(Response& response, std::string filename,
		const Request& request, CacheController::CachePragma cachePragma) {

	addCacheHeaders(response, filename, cachePragma);

	if (!response.headerExists("Content-Type")) {
		response.setHeader("Content-Type", getContentType(filename));
	}

	std::time_t timeVal = std::time(NULL);
	if (timeVal != -1 && !response.headerExists("Date")) {
		response.setHeader("Date", unixTimeToString(timeVal));
	}
}


void ResponseBuilder::addCacheHeaders(Response& response, std::string filename, CacheController::CachePragma pragma) const {
	response.setHeader("cache-control", cacheController.getValueFromPragma(pragma));

	if (pragma.isStore) {
		response.addHeader("etag", "\"" + pymlCache.getCacheTag(filename) + "\"");
	}
}


std::string ResponseBuilder::getContentType(std::string filename) {
	std::string extension;

	if (PythonModule::krait.checkIsNone("_content_type")) {
		DBG("No krait content_type set");
		bf::path filePath(filename);
		extension = filePath.extension().string();
		if (extension == ".pyml") {
			extension = filePath.stem().extension().string();
		}
	}
	else {
		std::string varContentType = PythonModule::krait.getGlobalStr("_content_type");
		DBG_FMT("response builder: content_type set to %1%", varContentType);

		if (!ba::starts_with(varContentType, "ext/")) {
			DBG_FMT("Content-type: %1%", varContentType);
			return varContentType;
		}
		else {
			extension = varContentType.substr(3); //strlen("ext")
			extension[0] = '.'; // /extension to .extension
		}
	}

	DBG_FMT("Extension: %1%", extension);

	auto it = contentTypeByExtension.find(extension);
	if (it == contentTypeByExtension.end()) {
		DBG("Content-type: application/octet-stream");
		return "application/octet-stream";
	}
	else {
		DBG_FMT("Content-type: %1%", it->second);
		return it->second;
	}
}


void ResponseBuilder::loadContentTypeList(std::string filename) {
	//bf::path contentFilePath = getExecRoot() / "globals" / "mime.types";
	std::ifstream mimeFile(filename);
	if (!mimeFile) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfoFromFormat("MIME types file missing! Filename: %1%", filename));
	}

	char line[1024];
	std::string mimeType;

	while (mimeFile.getline(line, 1023)) {
		if (line[0] == '\0' || line[0] == '#') {
			continue;
		}

		const char* separators = " \t\r\n";
		char* ptr = strtok(line, separators);
		if (ptr == NULL) {
			continue;
		}

		mimeType.assign(ptr);
		ptr = strtok(NULL, separators);
		while (ptr != NULL) {
			//This SHOULD be fine.
			//Make it an extension (html to .html)
			*(ptr - 1) = '.';
			contentTypeByExtension[std::string(ptr - 1)] = mimeType;

			ptr = strtok(NULL, separators);
		}

	}
}

std::string replaceParams(std::string target, std::map<std::string, std::string> params) {
	auto it = target.begin();
	auto oldIt = it;
	std::string result;

	while ((it = find(it, target.end(), '{')) != target.end()) {
		result += std::string(oldIt, it);
		auto endIt = find(it, target.end(), '}');
		if (endIt == target.end()) {
			BOOST_THROW_EXCEPTION(routeError() << stringInfoFromFormat("Error: unmatched paranthesis in route target %1%", target));
		}

		std::string paramName(it + 1, endIt);
		auto paramFound = params.find(paramName);
		if (paramFound == params.end()) {
			BOOST_THROW_EXCEPTION(routeError() << stringInfoFromFormat("Error: parameter name %1% in route target %2% not found",
				paramName, target));
		}
		result += paramFound->second;
		++it;
		oldIt = it;
	}
	result += std::string(oldIt, it);

	return result;
}

bool ResponseBuilder::pathBlocked(const std::string& path) {
	bf::path filePath(path);
	for (auto& part : filePath) {
		if (part.c_str()[0] == '.') {
			return true;
		}
	}
	return false;
}