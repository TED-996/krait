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


class Server
{
	static Server* instance;

	boost::filesystem::path serverRoot;
	int serverSocket;

	const int maxKeepAliveSec = 60;
	int keepAliveTimeoutSec;
	bool keepAlive;

	CacheController cacheController;
	std::unordered_map<std::string, std::string> contentTypeByExtension;

	int socketToClose;
	bool stdinDisconnected;

	StringPiper cacheRequestPipe;
	bool interpretCacheRequest;
	PymlCache serverCache;

	Config config;

	bool shutdownRequested;

	void tryAcceptConnection();
	void tryCheckStdinClosed() const;

	void serveClientStart(int clientSocket);
	void serveRequest(int clientSocket, Request& request);
	void addDefaultHeaders(Response& response, std::string filename, Request& request);
	Response getResponseFromSource(std::string filename, Request& request);

	std::string getFilenameFromTarget(std::string target);
	std::string expandFilename(std::string filename);
	static bool pathBlocked(std::string filename);

	std::string getContentType(std::string filename);
	void loadContentTypeList();

	void addStandardCacheHeaders(Response& response, std::string filename, CacheController::CachePragma pragma);

	bool canContainPython(std::string filename);
	void startWebsocketsServer(int clientSocket, Request& request);

	PymlFile* constructPymlFromFilename(std::string filename, boost::object_pool<PymlFile>& pool, char* tagDest);
	void onServerCacheMiss(std::string filename);

	bool getPymlIsDynamic(std::string filename);
	IteratorResult getPymlResultRequestCache(std::string filename);

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
