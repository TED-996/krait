#pragma once

#include<string>
#include<vector>
#include<unordered_set>
#include<unordered_map>
#include<time.h>
#include<memory>
#include<boost/filesystem/path.hpp>
#include<boost/pool/object_pool.hpp>
#include<signal.h>
#include"except.h"
#include"routes.h"
#include"network.h"
#include"pythonWorker.h"
#include"response.h"
#include"pyml.h"
#include"logger.h"
#include"stringPiper.h"


class Server {
	boost::filesystem::path serverRoot;
	std::vector<Route> routes;
	int serverSocket;

	const int maxKeepAliveSec = 60;
	int keepAliveTimeoutSec;
	bool keepAlive;

	StringPiper cacheRequestPipe;
	
	boost::object_pool<PymlFile> pymlPool;
	std::unordered_map<std::string, std::pair<std::time_t, PymlFile*>> pymlCache;
	std::unordered_map<std::string, std::pair<std::time_t, std::string>> rawFileCache;

	std::unordered_map<std::string, std::string> contentTypeByExtension;

	void tryAcceptConnection();
	static void tryWaitFinishedForks();
	void tryCachePymlFiles();
	void tryCheckStdinClosed();

	void serveClientStart(int clientSocket);
	void serveRequest(int clientSocket, Request& request);
	Response getResponseFromSource(std::string filename);
	void addDefaultHeaders(Response& response, std::string filename, Request& request);
	Response getResponseFromSource(std::string filename, Request& request);

	std::string getFilenameFromTarget(std::string target);
	std::string expandFilename(std::string filename);
	bool pathBlocked(std::string filename);

	
	std::string getRawFile(std::string filename);
	std::string getPymlResult(std::string filename);
	const PymlFile& addPymlToCache(std::string filename);
	std::string& addRawFileToCache(std::string filename);
	
	const PymlFile& getPymlRequestCache(std::string filename);
	std::string& getRawFileRequestCache(std::string filename);
	
	std::string getContentType(std::string filename);

	void loadContentTypeList();
	
	static bool canContainPython(std::string filename);

	static void initSignals();

	static int socketToClose;
	static bool clientWaitingResponse;
	static void signalStopRequested(int sig);
	static void signalKillRequested(int sig);
	static std::unordered_set<int> pids;

	bool stdinDisconnected;

public:
	Server(std::string serverRoot, int port);
	~Server();

	void runServer();
	void killChildren();
};
