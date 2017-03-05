#pragma once

#include<string>
#include<vector>
#include<unordered_set>
#include<unordered_map>
#include<boost/filesystem/path.hpp>
#include"except.h"
#include"routes.h"
#include"network.h"
#include"response.h"
#include"pyml.h"
#include"stringPiper.h"
#include"fileCache.h"
#include"cacheController.h"


class Server {
	boost::filesystem::path serverRoot;
	std::vector<Route> routes;
	int serverSocket;

	const int maxKeepAliveSec = 60;
	int keepAliveTimeoutSec;
	bool keepAlive;

	CacheController cacheController;
	std::unordered_map<std::string, std::string> contentTypeByExtension;

	void tryAcceptConnection();
	static void tryWaitFinishedForks();
	void tryCheckStdinClosed();

	void serveClientStart(int clientSocket);
	void serveRequest(int clientSocket, Request& request);
	Response getResponseFromSource(std::string filename);
	void addDefaultHeaders(Response& response, std::string filename, Request& request);
	Response getResponseFromSource(std::string filename, Request& request);

	std::string getFilenameFromTarget(std::string target);
	std::string expandFilename(std::string filename);
	bool pathBlocked(std::string filename);
	
	std::string getContentType(std::string filename);
	void loadContentTypeList();

	void addStandardCacheFields(Response& response, CacheController::CachePragma pragma);
	
	static bool canContainPython(std::string filename);

	static void initSignals();

	static int socketToClose;
	static bool clientWaitingResponse;
	static void signalStopRequested(int sig);
	static void signalKillRequested(int sig);
	static std::unordered_set<int> pids;

	static StringPiper cacheRequestPipe;
	static bool interpretCacheRequest;
	static FileCache<PymlFile> serverCache;
	static void updateParentCaches();

	static PymlFile* constructPymlFromString(std::string filename, boost::object_pool<PymlFile>& pool);
	static void onServerCacheMiss(std::string filename);

	static std::string getPymlResultRequestCache(std::string filename, bool *isDynamic);

	bool stdinDisconnected;

public:
	Server(std::string serverRoot, int port);
	~Server();

	void runServer();
	void killChildren();
};