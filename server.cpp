#include<unistd.h>
#include<sys/wait.h>
#include"server.h"
#include"except.h"
#include"pythonWorker.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;


Server::Server(string serverRoot, int port){
	this->serverRoot = path(serverRoot);
	this->routes = getRoutesFromFile((this->serverRoot / "config" / "routes.xml").string());
	this->serverSocket = getServerSocket(port, false);
	
	pythonInit(serverRoot);
}


void Server::runServer(){
	setSocketListen(this->serverSocket);
	
	while(true){
		tryAcceptConnection();
		tryWaitFinishedForks();
	}
}


void Server::tryAcceptConnection(){
	const int timeout = 100;
	int clientSocket = getNewClient(timeout);
	
	if (clientSocket == -1){
		return;
	}
	
	pid_t pid = fork();
	if (pid == -1){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Error: could not fork()! Is the system out of resources?") << errcodeInfo(errno));
	}
	if (pid == 0){
		serveClientStart();//TODO: connect stderr?
		exit(255); //The function above should call exit()!
	}
	
	pids.insert((int)pid);
}


void Server::tryWaitFinishedForks(){
	if (pids.count() == 0){
		return;
	}
	
	int status;
	pid_t pid;
	while((pid = waitpid(-1, &status, WNOHANG) != 0)){
		if (WIFEXITED(status) || WIFSIGNALED(status) || WIFSTOPPED(status)){
			pids.erase((int)pid);
		}
	}
}


void Server::serveClientStart(int clientSocket){

}

