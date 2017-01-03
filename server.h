#pragma once

#include<string>
#include<vector>
#include<unordered_set>
#include<unordered_map>
#include<time.h>
#include<boost/filesystem/path.hpp>
#include<boost/pool/object_pool.hpp>
#include"except.h"
#include"routes.h"
#include"network.h"
#include"pythonWorker.h"
#include"response.h"
#include"pyml.h"
#include"logger.h"


class Server {
	boost::filesystem::path serverRoot;
	std::vector<Route> routes;
	int serverSocket;
	std::unordered_set<int> pids;

	LoggerIn infoLogger;
	LoggerIn errLogger;
	
	boost::object_pool<PymlFile> pymlPool;
	std::unordered_map<std::string, std::pair<std::time_t, PymlFile*>> pymlCache;
	std::unordered_map<std::string, std::pair<std::time_t, std::string>> rawFileCache;

	std::unordered_map<std::string, std::string> contentTypeByExtension;

	void tryAcceptConnection();
	void tryWaitFinishedForks();

	void serveClientStart(int clientSocket);
	Response getResponseFromSource(std::string filename);
	void addDefaultHeaders(Response& response);
	Response getResponseFromSource(std::string filename, Request& request);

	std::string getFilenameFromTarget(std::string target);
	std::string expandFilename(std::string filename);
	bool pathBlocked(std::string filename);

	
	std::string getRawFile(std::string filename);
	std::string getPymlResult(std::string filename);
	const PymlFile& addPymlToCache(std::string filename);
	std::string& addRawFileToCache(std::string filename);
	
	std::string getContentType(std::string filename);

	void loadContentTypeList();

public:
	Server(std::string serverRoot, int port, LoggerIn infoLogger, LoggerIn errLogger);
	~Server();

	void runServer();
	void killChildren();
};
