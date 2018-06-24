#pragma once
#include "cacheController.h"
#include "pymlCache.h"
#include "pythonApiManager.h"
#include "request.h"
#include "response.h"
#include <boost/filesystem/path.hpp>

class ResponseBuilder {
private:
    boost::filesystem::path siteRoot;
    Config& config;
    CacheController& cacheController;
    PymlCache& pymlCache;

    SourceConverter converter;
    CompilerDispatcher compiler;

    PyEmitModule emitModule;

    PymlRenderer pymlRenderer;
    CompiledPythonRenderer compiledRenderer;
    RendererDispatcher renderer;

    PyCompileModule compileModule;
    PythonApiManager apiManager;

    std::unordered_map<std::string, std::string> contentTypeByExtension;

    ResponseSource getSourceFromRequest(const Request& request) const;
    std::string getFilenameFromTarget(const std::string& target) const;
    static bool pathBlocked(const std::string& path);
    std::string expandFilename(const std::string& filename) const;
    std::string getModuleNameFromFilename(boost::string_ref filename) const;
    std::string getModuleNameFromMvcRoute(const boost::python::object& obj, size_t routeIdx) const;

    const IPymlFile& getPymlFromCache(const std::string& filename) const;

    void addDefaultHeaders(Response& response,
        const std::string& filename,
        const Request& request,
        CacheController::CachePragma cachePragma,
        bool isDynamic);
    void addCacheHeaders(Response& response, const std::string& filename, CacheController::CachePragma pragma) const;

    std::string getContentType(const std::string& filename, bool isDynamic);
    void loadContentTypeList(const std::string& filename);

    std::unique_ptr<Response> buildResponseInternal(const Request& request, bool isWebsockets);

public:
    ResponseBuilder(const boost::filesystem::path& siteRoot,
        Config& config,
        CacheController& cacheController,
        PymlCache& pymlCache);

    std::unique_ptr<Response> buildResponse(Request& request);
    std::unique_ptr<Response> buildWebsocketsResponse(Request& request);
};
