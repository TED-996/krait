#pragma once

#include<string>
#include<vector>
#include<unordered_set>
#include<unordered_map>
#include<boost/filesystem/path.hpp>
#include"except.h"
#include"routes.h"
#include"network.h"
#include"pythonWorker.h"
#include"response.h"
#include"logger.h"


class Server {
	boost::filesystem::path serverRoot;
	std::vector<Route> routes;
	int serverSocket;
	std::unordered_set<int> pids;
	
	LoggerIn infoLogger;
	LoggerIn errLogger;
	
	std::unordered_map<std::string, std::string> contentTypeByExtension;
	
	void tryAcceptConnection();
	void tryWaitFinishedForks();
	
	void serveClientStart(int clientSocket);
	Response getResponseFromSource(std::string filename);
	void addDefaultHeaders(Response& response);
	Response getResponseFromSource(std::string filename, Request& request);
	
	std::string getSourceFromTarget(std::string target);
	std::string expandFilename(std::string filename);
	std::string getHtmlSource(std::string filename);
	bool pathBlocked(std::string filename);
	std::string getContentType(std::string filename);
	
	void loadContentTypeList();
	
public:
	Server(std::string serverRoot, int port, LoggerIn infoLogger, LoggerIn errLogger);
	~Server();
	
	void runServer();
	void killChildren();
};
