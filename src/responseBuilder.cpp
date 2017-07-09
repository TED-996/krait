#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "responseBuilder.h"
#include "pythonModule.h"
#include "utils.h"

#define DBG_DISABLE
#include "dbg.h"

namespace bf = boost::filesystem;
namespace ba = boost::algorithm;


void ResponseBuilder::addDefaultHeaders(Response& response, std::string filename, Request& request) {
	if (!response.headerExists("Content-Type")) {
		response.setHeader("Content-Type", getContentType(filename));
	}
	std::time_t timeVal = std::time(NULL);
	if (timeVal != -1 && !response.headerExists("Date")) {
		response.setHeader("Date", unixTimeToString(timeVal));
	}
}


void ResponseBuilder::addStandardCacheHeaders(Response& response, std::string filename, CacheController::CachePragma pragma) {
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
