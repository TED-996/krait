#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "responseBuilder.h"
#include "pythonModule.h"
#include "utils.h"
#include "path.h"
#include "logger.h"
#include "mvcPymlFile.h"


//#define DBG_DISABLE
#include "dbg.h"
namespace bf = boost::filesystem;
namespace ba = boost::algorithm;


ResponseBuilder::ResponseBuilder(const boost::filesystem::path& siteRoot, Config& config, CacheController& cacheController, PymlCache& pymlCache)
	: config(config),
	  cacheController(cacheController),
	  pymlCache(pymlCache),
	  siteRoot(siteRoot) {
	loadContentTypeList((getShareRoot() / "globals" / "mime.types").string());
}


std::unique_ptr<Response> ResponseBuilder::buildResponseInternal(const Request& request, bool isWebsockets) {
	bool isHead = (request.getVerb() == HttpVerb::HEAD);
	std::string symFilename = "<unavailable>";
	std::unique_ptr<Response> response = nullptr;

	try {
		PreResponseSource source = std::move(getSourceFromRequest(request));
		
		symFilename = source.symFilename;
		bool isDynamic;
		
		std::string etag;
		if (request.headerExists("if-none-match")) {
			etag = request.getHeader("if-none-match").get();
			etag = etag.substr(1, etag.length() - 2);
		}

		boost::optional<const IPymlFile&> pymlFile;
		std::unique_ptr<MvcPymlFile> mvcFileStorage = nullptr;

		if (source.sourceFilename) {
			pymlFile = getPymlFromCache(source.sourceFilename.get());
			isDynamic = pymlFile->isDynamic();
		}
		else if(source.sourceObject) {
			isDynamic = true;
			mvcFileStorage = std::make_unique<MvcPymlFile>(source.sourceObject.get(), pymlCache);
			pymlFile = *mvcFileStorage;
		}
		else {
			BOOST_THROW_EXCEPTION(serverError() << stringInfo("ResponseBuilder::getSourceFromRequest returned empty PreResponseSource."));
		}
		
		CacheController::CachePragma cachePragma = cacheController.getCacheControl(source.symFilename, !isDynamic);
		
		if (cachePragma.isStore && pymlCache.checkCacheTag(source.symFilename, etag)) {
			response = std::make_unique<Response>(304, "", false);
		}
		else {
			if (isDynamic) {
				apiManager.set(request, isWebsockets);
			}

			response = std::move(renderer.render(*pymlFile, request));

			if (isDynamic) {
				if (apiManager.isCustomResponse()) {
					response = std::move(apiManager.getCustomResponse());
				}
				apiManager.addHeaders(*response);
			}
		}

		addDefaultHeaders(*response, source.symFilename, request, cachePragma, isDynamic);
	}
	catch(notFoundError&) {
		Loggers::logInfo(formatString("Not found building %1%", symFilename)); //TODO: encapsulate some info in NotFoundError
		response = std::make_unique<Response>(404, "<html><body><h1>404 Not Found</h1></body></html>", true);
	}

	if (isHead) {
		response->setBody("", false);
	}

	return response;
}

std::unique_ptr<Response> ResponseBuilder::buildResponse(Request& request) {
	return buildResponseInternal(request, false);
}

std::unique_ptr<Response> ResponseBuilder::buildWebsocketsResponse(Request& request) {
	request.setRouteVerb(RouteVerb::WEBSOCKET);
	
	return buildResponseInternal(request, true);
}

std::string replaceParams(const std::string& target, std::map<std::string, std::string>& params);

ResponseBuilder::PreResponseSource ResponseBuilder::getSourceFromRequest(const Request& request) const {
	std::map<std::string, std::string> params;
	const Route& route = Route::getRouteMatch(config.getRoutes(), request.getRouteVerb(), request.getUrl(), params);
	
	std::string symFilename;

	if (!route.isMvcRoute()) {
		std::string targetSource = route.getTarget(request.getUrl());
		if (!params.empty()) {
			targetSource = replaceParams(targetSource, params);
		}
		symFilename = expandFilename(getFilenameFromTarget(targetSource));
	}
	else {
		symFilename = getFilenameFromTarget(request.getUrl());
	}

	if (pathBlocked(symFilename)) {
		Loggers::logInfo(formatString("Client tried accessing path %1%, BLOCKED (404).", symFilename));
		BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Error: File not found: %1%", symFilename));
	}

	if (!route.isMvcRoute()) {
		return PreResponseSource(symFilename, symFilename);
	}
	else {
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


std::string ResponseBuilder::expandFilename(const std::string& sourceFilename) const {
	std::string workingFilename = sourceFilename;
	if (bf::is_directory(workingFilename)) {
		//Converting to directory automatically.
		//Not index.html; taking care below about it (might be .pyml or .py, for example)
		workingFilename = (bf::path(workingFilename) / "index").string();
	}

	if (!bf::exists(workingFilename)) {
		if (bf::exists(workingFilename + ".html")) {
			workingFilename += ".html";
		}
		else if (bf::exists(workingFilename + ".htm")) {
			workingFilename += ".htm";
		}
		else if (bf::exists(workingFilename + ".pyml")) {
			workingFilename += ".pyml";
		}
		else if (bf::exists(workingFilename + ".py")) {
			workingFilename += ".py";
		}

		else if (bf::path(workingFilename).extension() == ".html" &&
			bf::exists(bf::change_extension(workingFilename, ".htm"))) {
			workingFilename = bf::change_extension(workingFilename, "htm").string();
		}
		else if (bf::path(workingFilename).extension() == ".htm" &&
			bf::exists(bf::change_extension(workingFilename, ".html"))) {
			workingFilename = bf::change_extension(workingFilename, "html").string();
		}
	}
	return workingFilename;
}

const IPymlFile& ResponseBuilder::getPymlFromCache(const std::string& filename) const {
	return pymlCache.get(filename);
}
 
void ResponseBuilder::addDefaultHeaders(Response& response, const std::string& filename,
		const Request& request, CacheController::CachePragma cachePragma, bool isDynamic) {

	addCacheHeaders(response, filename, cachePragma);

	if (!response.headerExists("Content-Type")) {
		response.setHeader("Content-Type", getContentType(filename, isDynamic));
	}

	std::time_t timeVal = std::time(nullptr);
	if (timeVal != -1 && !response.headerExists("Date")) {
		response.setHeader("Date", unixTimeToString(timeVal));
	}
}

void ResponseBuilder::addCacheHeaders(Response& response, const std::string& filename, CacheController::CachePragma pragma) const {
	response.setHeader("cache-control", cacheController.getValueFromPragma(pragma));

	if (pragma.isStore) {
		response.addHeader("etag", "\"" + pymlCache.getCacheTag(filename) + "\"");
	}
}


std::string ResponseBuilder::getContentType(const std::string& filename, bool isDynamic) {
	std::string extension;

	if (!isDynamic || PythonModule::krait.checkIsNone("_content_type")) {
		bf::path filePath(filename);
		extension = filePath.extension().string();
		if (extension == ".pyml") {
			extension = filePath.stem().extension().string();
		}
	}
	else {
		std::string varContentType = PythonModule::krait.getGlobalStr("_content_type");
		
		if (!ba::starts_with(varContentType, "ext/")) {
			return varContentType;
		}
		else {
			extension = varContentType.substr(3); //strlen("ext")
			extension[0] = '.'; // /"extension" to ".extension"
		}
	}

	const auto& it = contentTypeByExtension.find(extension);
	if (it == contentTypeByExtension.end()) {
		return "application/octet-stream";
	}
	else {
		return it->second;
	}
}


void ResponseBuilder::loadContentTypeList(const std::string& filename) {
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
		if (ptr == nullptr) {
			continue;
		}

		mimeType.assign(ptr);
		ptr = strtok(nullptr, separators);
		while (ptr != nullptr) {
			//This SHOULD be fine. This ptr is definitely not the first character in the char[].
			//Make it an extension (html to .html)
			*(ptr - 1) = '.';
			contentTypeByExtension[std::string(ptr - 1)] = mimeType;

			ptr = strtok(nullptr, separators);
		}
	}
}

std::string replaceParams(const std::string& target, std::map<std::string, std::string>& params) {
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