#pragma once

#include<string>
#include<vector>
#include<unordered_set>
#include<unordered_map>
#include<time.h>
#include<memory>
#include<boost/filesystem/path.hpp>
#include<boost/pool/object_pool.hpp>
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
	std::unordered_set<int> pids;

	LoggerIn infoLogger;
	LoggerIn errLogger;
	StringPiper cacheRequestPipe;
	
	boost::object_pool<PymlFile> pymlPool;
	std::unordered_map<std::string, std::pair<std::time_t, PymlFile*>> pymlCache;
	std::unordered_map<std::string, std::pair<std::time_t, std::string>> rawFileCache;

	std::unordered_map<std::string, std::string> contentTypeByExtension;

	void tryAcceptConnection();
	void tryWaitFinishedForks();
	void tryCachePymlFiles();

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

public:
	Server(std::string serverRoot, int port, LoggerIn infoLogger, LoggerIn errLogger);
	~Server();

	void runServer();
	void killChildren();
};
