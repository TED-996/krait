#pragma once

#include <string>
#include <unordered_map>
#include <boost/filesystem/path.hpp>
#include "except.h"
#include "network.h"
#include "response.h"
#include "pymlFile.h"
#include "stringPiper.h"
#include "pymlCache.h"
#include "cacheController.h"
#include "config.h"
#include "responseBuilder.h"


class Server
{
	static Server* instance;

	boost::filesystem::path serverRoot;
	int serverSocket;

	const int maxKeepAliveSec = 60;
	int keepAliveTimeoutSec;
	bool keepAlive;

	Config config;
	CacheController cacheController;

	int socketToClose;
	bool stdinDisconnected;

	StringPiper cacheRequestPipe;
	bool interpretCacheRequest;
	PymlCache serverCache;

	ResponseBuilder responseBuilder;

	bool shutdownRequested;

	void tryAcceptConnection();
	void tryCheckStdinClosed() const;

	void serveClientStart(int clientSocket);
	void serveRequest(int clientSocket, Request& request);

	bool canContainPython(std::string filename);
	void serveRequestWebsockets(int clientSocket, Request& request);

	std::unique_ptr<PymlFile> constructPymlFromFilename(std::string filename, PymlCache::CacheTag& tagDest);
	void onServerCacheMiss(std::string filename);

	void updateParentCaches();

public:
	Server(std::string serverRoot, int port);
	~Server();

	void runServer();

	void requestShutdown();
	void cleanup();

	static Server* getInstance() {
		return instance;
	}
};
