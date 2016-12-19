#pragma once

#include<string>
#include<vector>
#include<unordered_set>
#include<boost/filesystem/path.hpp>
#include"except.h"
#include"routes.h"
#include"network.h"
#include"pythonWorker.h"

class Server {
	boost::filesystem::path serverRoot;
	std::vector<Route> routes;
	int serverSocket;
	std::unordered_set<int> pids;
	
	void tryAcceptConnection();
	void tryWaitFinishedForks();
	
	void serveClientStart(int clientSocket);
	
public:
	Server(std::string serverRoot, int port);
	~Server();
	
	void runServer();
};
