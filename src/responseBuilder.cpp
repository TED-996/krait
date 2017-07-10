#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "responseBuilder.h"
#include "pythonModule.h"
#include "utils.h"

#define DBG_DISABLE
#include "dbg.h"
#include "logger.h"

namespace bf = boost::filesystem;
namespace ba = boost::algorithm;


Response ResponseBuilder::buildResponse(Request& request) {
	Response response(500, "", true);

	bool isHead = (request.getVerb() == HttpVerb::HEAD);

	try {

		std::string filename = getSourceFromRequest(request);
		const IPymlFile& pymlFile = getPymlFromCache(filename);
		bool isDynamic = pymlFile.isDynamic();
		CacheController::CachePragma cachePragma = cacheController.getCacheControl(filename, !isDynamic);

		std::string etag;
		if (request.headerExists("if-none-match")) {
			etag = request.getHeader("if-none-match").get();
			etag = etag.substr(1, etag.length() - 2);
		}

		if (cachePragma.isStore && pymlCache.checkCacheTag(filename, etag)) {
			response = Response(304, "", false);
		}

		addDefaultHeaders(response, filename, request, cachePragma);
	}

	if (isHead) {
		response.setBody("", false);
	}

	return response;
}

std::string replaceParams(std::string target, std::map<std::string, std::string> params);

std::string ResponseBuilder::getSourceFromRequest(Request& request) {
	std::map<std::string, std::string> params;
	const Route& route = Route::getRouteMatch(config.getRoutes(), request.getRouteVerb(), request.getUrl(), params);
	
	bool isFromFile;
	std::string sourceFile;
	if (!route.isMvcRoute()) {
		std::string targetReplaced = replaceParams(route.getTarget(request.getUrl()), params);
		sourceFile = getFilenameFromTarget(targetReplaced);
		isFromFile = false;
	}
	else {
		//TODO: see how to set better... Will not be working for now.
		//WARNING: NOT OK at the moment: caching/etc is looked up for the TEMPLATE, not the URL!
		//PythonModule::main.setGlobal("ctrl", PythonModule::main.callObject(route.getCtrlClass()));
		//sourceFile = PythonModule::main.eval("krait.get_full_path(ctrl.get_view())");
		//TODO: struct ResponseSource { symFilename, (filename | mvcControllerClass) }
		isFromFile = true;
	}

	if (!isFromFile && pathBlocked(sourceFile)) {
		Loggers::logInfo(formatString("Tried accessing path %1%, BLOCKED.", sourceFile));
		BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Error: File not found: %1%", sourceFile));
	}

	std::string filename = expandFilename(sourceFile);

	return filename;
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
		DBG("No content_type set");
		bf::path filePath(filename);
		extension = filePath.extension().string();
		if (extension == ".pyml") {
			extension = filePath.stem().extension().string();
		}
	}
	else {
		std::string varContentType = PythonModule::krait.getGlobalStr("_content_type");
		DBG_FMT("content_type set to %1%", varContentType);

		if (!ba::starts_with(varContentType, "ext/")) {
			return varContentType;
		}
		else {
			extension = varContentType.substr(3); //strlen("ext")
			extension[0] = '.'; // /extension to .extension
		}
	}

	//DBG_FMT("Extension: %1%", extension);

	auto it = contentTypeByExtension.find(extension);
	if (it == contentTypeByExtension.end()) {
		return "application/octet-stream";
	}
	else {
		return it->second;
	}
}


void ResponseBuilder::loadContentTypeList(std::string filename) {
	//bf::path contentFilePath = getExecRoot() / "globals" / "mime.types";
	std::ifstream mimeFile(filename);

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