#pragma once

#include "IManagedSocket.h"
#include "cacheController.h"
#include "config.h"
#include "except.h"
#include "networkManager.h"
#include "pymlCache.h"
#include "pymlFile.h"
#include "pythonInitializer.h"
#include "responseBuilder.h"
#include "stringPiper.h"
#include <boost/filesystem/path.hpp>
#include <string>


class Server {
    static Server* instance;

    boost::filesystem::path serverRoot;

    const int maxKeepAliveSec = 60;
    int keepAliveTimeoutSec;
    bool keepAlive;

    PythonInitializer pythonInitializer;
    Config config;
    CacheController cacheController;

    NetworkManager networkManager;
    std::unique_ptr<IManagedSocket> clientSocket;

    bool stdinDisconnected;

    StringPiper cacheRequestPipe;
    bool interpretCacheRequest;
    PymlCache serverCache;

    ResponseBuilder responseBuilder;

    bool shutdownRequested;

    void tryAcceptConnection();
    void tryCheckStdinClosed() const;

    void serveClientStart();
    void serveRequest(Request& request);

    bool canContainPython(const std::string& filename);
    void serveRequestWebsockets(Request& request);

    std::unique_ptr<PymlFile> constructPymlFromFilename(const std::string& filename, PymlCache::CacheTag& tagDest);
    void onServerCacheMiss(const std::string& filename);

    void updateParentCaches();

public:
    explicit Server(ArgvConfig argvConfig);
    ~Server();

    void runServer();

    void requestShutdown();
    void cleanup();

    static Server* getInstance() {
        return instance;
    }
};
