#include "responseBuilder.h"
#include "logger.h"
#include "mvcPymlFile.h"
#include "path.h"
#include "pythonModule.h"
#include "utils.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <fstream>


//#define DBG_DISABLE
#include "dbg.h"
namespace bf = boost::filesystem;
namespace ba = boost::algorithm;


ResponseBuilder::ResponseBuilder(
    const boost::filesystem::path& siteRoot, Config& config, CacheController& cacheController, PymlCache& pymlCache)
        : siteRoot(siteRoot)
        , config(config)
        , cacheController(cacheController)
        , pymlCache(pymlCache)
        , converter(siteRoot, pymlCache)
        , compiler(siteRoot, pymlCache, converter)
        , emitModule()
        , pymlRenderer(emitModule)
        , compiledRenderer(emitModule)
        , renderer(pymlCache, pymlRenderer, compiledRenderer, converter)
        , compileModule(compiler, renderer, converter)
        , apiManager(emitModule, compileModule) {
    loadContentTypeList((getShareRoot() / "globals" / "mime.types").string());
}


std::unique_ptr<Response> ResponseBuilder::buildResponseInternal(const Request& request, bool isWebsockets) {
    bool isHead = (request.getVerb() == HttpVerb::HEAD);
    std::string baseFilename = "<unavailable>";
    std::unique_ptr<Response> response = nullptr;

    try {
        ResponseSource source = getSourceFromRequest(request);

        baseFilename = source.getBaseFilename();
        bool isDynamic = source.isDynamic();

        std::string etag;
        if (request.headerExists("if-none-match")) {
            etag = request.getHeader("if-none-match").get();
            etag = etag.substr(1, etag.length() - 2);
        }

        CacheController::CachePragma cachePragma = cacheController.getCacheControl(baseFilename, !isDynamic);

        if (cachePragma.isStore && pymlCache.checkCacheTag(baseFilename, etag)) {
            response = std::make_unique<Response>(304, "", false);
        } else {
            if (isDynamic) {
                if (!isWebsockets) {
                    apiManager.setRegularRequest(request);
                } else {
                    apiManager.setWebsocketsRequest(request);
                }
            }

            response = std::make_unique<Response>(200, renderer.get(source), false);

            if (isDynamic) {
                if (apiManager.isCustomResponse()) {
                    response = apiManager.getCustomResponse();
                }
                apiManager.addHeaders(*response);
            }
        }

        addDefaultHeaders(*response, baseFilename, request, cachePragma, isDynamic);
    } catch (notFoundError&) {
        Loggers::logInfo(formatString("Not found building %1%", baseFilename));
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

ResponseSource ResponseBuilder::getSourceFromRequest(const Request& request) const {
    std::map<std::string, std::string> params;
    const Route& route = Route::getRouteMatch(config.getRoutes(), request.getRouteVerb(), request.getUrl(), params);
    size_t routeIdx = 0;

    // Find the route, get its index
    // Bit of a hack, maybe should change the API
    for (auto it = config.getRoutes().begin(); it != config.getRoutes().end(); ++it) {
        if (&route == &*it) {
            routeIdx = it - config.getRoutes().begin();
        }
    }

    std::string baseFilename;

    if (!route.isMvcRoute()) {
        std::string targetSource = route.getTarget(request.getUrl());
        if (!params.empty()) {
            targetSource = replaceParams(targetSource, params);
        }
        baseFilename = expandFilename(getFilenameFromTarget(targetSource));
    } else {
        baseFilename = getFilenameFromTarget(request.getUrl());
    }

    if (pathBlocked(baseFilename)) {
        Loggers::logInfo(formatString("Client tried accessing path %1%, BLOCKED (404).", baseFilename));
        BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Error: File not found: %1%", baseFilename));
    }

    ResponseSource result(baseFilename);

    if (!route.isMvcRoute()) {
        result.setSourceFilename(std::move(baseFilename));
        converter.extend(result);
        return result;
    } else {
        result.setMvcController(route.getCtrlClass());
        result.setModuleName(getModuleNameFromMvcRoute(route.getCtrlClass(), routeIdx));
        return result;
    }
}


std::string ResponseBuilder::getFilenameFromTarget(const std::string& target) const {
    if (target[0] == '/') {
        return (siteRoot / target.substr(1)).string();
    } else {
        return (siteRoot / target).string();
    }
}


std::string ResponseBuilder::expandFilename(const std::string& sourceFilename) const {
    std::string workingFilename = sourceFilename;
    if (bf::is_directory(workingFilename)) {
        // Converting to directory automatically.
        // Not index.html; taking care below about it (might be .pyml or .py, for example)
        workingFilename = (bf::path(workingFilename) / "index").string();
    }

    if (!bf::exists(workingFilename)) {
        if (bf::exists(workingFilename + ".html")) {
            workingFilename += ".html";
        } else if (bf::exists(workingFilename + ".htm")) {
            workingFilename += ".htm";
        } else if (bf::exists(workingFilename + ".pyml")) {
            workingFilename += ".pyml";
        } else if (bf::exists(workingFilename + ".py")) {
            workingFilename += ".py";
        }

        else if (bf::path(workingFilename).extension() == ".html"
            && bf::exists(bf::change_extension(workingFilename, ".htm"))) {
            workingFilename = bf::change_extension(workingFilename, "htm").string();
        } else if (bf::path(workingFilename).extension() == ".htm"
            && bf::exists(bf::change_extension(workingFilename, ".html"))) {
            workingFilename = bf::change_extension(workingFilename, "html").string();
        }
    }
    return workingFilename;
}

std::string ResponseBuilder::getModuleNameFromFilename(boost::string_ref filename) const {
    return converter.sourceToModuleName(filename);
}

std::string ResponseBuilder::getModuleNameFromMvcRoute(const boost::python::object& obj, size_t routeIdx) const {
    return "_krait_compiled._mvc_compiled.mvc_compiled_" + std::to_string(routeIdx);
}

const IPymlFile& ResponseBuilder::getPymlFromCache(const std::string& filename) const {
    return pymlCache.get(filename);
}

void ResponseBuilder::addDefaultHeaders(Response& response,
    const std::string& filename,
    const Request& request,
    CacheController::CachePragma cachePragma,
    bool isDynamic) {
    addCacheHeaders(response, filename, cachePragma);

    if (!response.headerExists("Content-Type")) {
        response.setHeader("Content-Type", getContentType(filename, isDynamic));
    }

    std::time_t timeVal = std::time(nullptr);
    if (timeVal != -1 && !response.headerExists("Date")) {
        response.setHeader("Date", unixTimeToString(timeVal));
    }
}

void ResponseBuilder::addCacheHeaders(
    Response& response, const std::string& filename, CacheController::CachePragma pragma) const {
    response.setHeader("cache-control", cacheController.getValueFromPragma(pragma));

    if (pragma.isStore) {
        response.addHeader("etag", "\"" + pymlCache.getCacheTag(filename) + "\"");
    }
}


std::string ResponseBuilder::getContentType(const std::string& filename, bool isDynamic) {
    std::string extension;

    boost::optional<std::string> customContentType = boost::none;
    if (isDynamic) {
        customContentType = apiManager.getCustomContentType();
    }
    if (!customContentType) {
        bf::path filePath(filename);
        extension = filePath.extension().string();
        if (extension == ".pyml") {
            extension = filePath.stem().extension().string();
        }
    } else {
        if (!ba::starts_with(customContentType.get(), "ext/")) {
            return std::move(customContentType.get());
        } else {
            extension = customContentType->substr(3); // strlen("ext")
            extension[0] = '.'; // /"extension" to ".extension"
        }
    }

    const auto& it = contentTypeByExtension.find(extension);
    if (it == contentTypeByExtension.end()) {
        return "application/octet-stream";
    } else {
        return it->second;
    }
}


void ResponseBuilder::loadContentTypeList(const std::string& filename) {
    std::ifstream mimeFile(filename);
    if (!mimeFile) {
        BOOST_THROW_EXCEPTION(
            serverError() << stringInfoFromFormat("MIME types file missing! Filename: %1%", filename));
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
            // This SHOULD be fine. This ptr is definitely not the first character in the char[].
            // Make it an extension (html to .html)
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
            BOOST_THROW_EXCEPTION(
                routeError() << stringInfoFromFormat("Error: unmatched paranthesis in route target %1%", target));
        }

        std::string paramName(it + 1, endIt);
        auto paramFound = params.find(paramName);
        if (paramFound == params.end()) {
            BOOST_THROW_EXCEPTION(routeError()
                << stringInfoFromFormat("Error: parameter name %1% in route target %2% not found", paramName, target));
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
        if (part.size() > 1 && part.c_str()[0] == '.') {
            return true;
        }
    }
    return false;
}