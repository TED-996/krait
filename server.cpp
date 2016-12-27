#include<unistd.h>
#include<sys/wait.h>
#include<signal.h>
#include<algorithm>
#include<fstream>
#include"server.h"
#include"except.h"
#include"pythonWorker.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;


Server::Server(string serverRoot, int port, LoggerIn infoLogger, LoggerIn errLogger)
	: infoLogger(infoLogger), errLogger(errLogger) {
	this->serverRoot = path(serverRoot);
	this->routes = getRoutesFromFile((this->serverRoot / "config" / "routes.xml").string());
	this->serverSocket = getServerSocket(port, false);
	
	pythonInit(serverRoot);
	
	infoLogger.log("Server initialized.");
}


Server::~Server(){
	//TODO: some signaling?
	close(serverSocket);
	while (pids.size() != 0){
		tryWaitFinishedForks();
		sleep(1);
	}
	infoLogger.log("Server destructed.");
}


void Server::killChildren(){
	for (int pid : pids){
		kill((pid_t)pid, SIGTERM);
	}
	infoLogger.log("Chilren killed.");
}


void Server::runServer(){
	setSocketListen(this->serverSocket);
	infoLogger.log("Server running");
	
	while(true){
		tryAcceptConnection();
		tryWaitFinishedForks();
	}
}


void Server::tryAcceptConnection(){
	const int timeout = 100;
	int clientSocket = getNewClient(serverSocket, timeout);
	
	if (clientSocket == -1){
		return;
	}
	
	pid_t pid = fork();
	if (pid == -1){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Error: could not fork()! Is the system out of resources?") << errcodeInfo(errno));
	}
	if (pid == 0){
		closeSocket(serverSocket);
		serveClientStart(clientSocket);//TODO: connect stderr?
		exit(255); //The function above should call exit()!
	}
	
	pids.insert((int)pid);
}


void Server::tryWaitFinishedForks(){
	if (pids.size() == 0){
		return;
	}
	
	int status;
	pid_t pid;
	while((pid = waitpid(-1, &status, WNOHANG) != 0)){
		if (WIFEXITED(status) || WIFSIGNALED(status) || WIFSTOPPED(status)){
			pids.erase((int)pid);
		}map<HttpVerb, string> verbToStrMapping {
		{HttpVerb::ANY, "ANY"},
		{HttpVerb::GET, "GET"},
		{HttpVerb::HEAD, "HEAD"},
		{HttpVerb::POST, "POST"},
		{HttpVerb::PUT, "PUT"},
		{HttpVerb::DELETE, "DELETE"},
		{HttpVerb::CONNECT, "CONNECT"},
		{HttpVerb::OPTIONS, "OPTIONS"},
		{HttpVerb::TRACE, "TRACE"}
	};
	}
}


string replaceParams(string target, map<string, string> params);

void Server::serveClientStart(int clientSocket){
	infoLogger.log("Serving a new client");
	//TODO: handle SIGTERM in some way...
	//TODO: handle execptions (return 5xx + log)
	Response resp(1, 1, 500, unordered_map<string, string>(), string());

	bool isHead = false;
	
	try{
		Request clientRequest = getRequestFromSocket(clientSocket);
		
		map<string, string> params;
		Route route = getRouteMatch(routes, clientRequest.getVerb(), clientRequest.getUrl(), params);
		
		if (clientRequest.getVerb() == HttpVerb::HEAD){
			isHead = true;
			clientRequest.setVerb(HttpVerb::GET);
		}
		
		string sourceFile;
		if (route.isDefaultTarget()){
			sourceFile = getSourceFromTarget(clientRequest.getUrl());
		}
		else{
			string targetReplaced = replaceParams(route.getTarget(), params);
			sourceFile = getSourceFromTarget(route.getTarget());
		}
		
		pythonSetGlobalRequest("request", clientRequest);
		pythonSetGlobal("url_params", params);
		pythonSetGlobal("resp_headers", map<string, string>());
		
		resp = getResponseFromSource(sourceFile, clientRequest);
	}
	catch(notFoundError&){
		resp = Response(1, 1, 404, unordered_map<string, string>(), string());
	}
	catch(rootException& ex){
		resp = Response(1, 1, 500, unordered_map<string, string>(), string());
		errLogger.log(formatString("Error serving client: %1%", ex.what()));
	}
	
	if (isHead){
		resp.setBody(string());
	}
	
	respondWithObject(clientSocket, resp);
	
	closeSocket(clientSocket);
	
	//TODO: recycle connection if necessary
	
	exit(0);
}


std::string Server::getSourceFromTarget(std::string target) {
	if (target[0] == '/'){
		return (serverRoot / target.substr(1)).string();
	}
	else{
		return (serverRoot / target).string();
	}
}


string replaceParams(string target, map<string, string> params) {
	auto it = target.begin();
	auto oldIt = it;
	string result;
	
	while((it = find(it, target.end(), '{')) != target.end()){
		result += string(oldIt, it);
		auto endIt = find(it, target.end(), '}');
		if (endIt == target.end()){
			BOOST_THROW_EXCEPTION(routeError() << stringInfoFromFormat("Error: unmatched paranthesis in route target %1%", target));
		}
		
		string paramName(it + 1, endIt);
		auto paramFound = params.find(paramName);
		if (paramFound == params.end()){
			BOOST_THROW_EXCEPTION(routeError() << stringInfoFromFormat("Error: parameter name %1% in route target %2% not found", paramName, target));
		}
		result += paramFound->second;
		it++;
		oldIt = it;
	}
	result += string(oldIt, it);
	
	return result;
}


string getHtmlSource(string filename);

Response Server::getResponseFromSource(string filename, Request& request){
	//HTML:	<@ @> => echo
	//		<! !> => run

	string htmlSource = getHtmlSource(filename);
	
	auto oldIt = htmlSource.begin();
	auto it = find(oldIt, htmlSource.end(), '<');
	string responseData;
	responseData.reserve(htmlSource.length());
	
	while(it != htmlSource.end()){
		responseData += string(oldIt, it);
		
		if (it + 1 != htmlSource.end()){
			char tagFirstChr = it[1];
			char startIdx = it - htmlSource.begin();
			char endTag[3] = "?>";
			if ( tagFirstChr == '@' || tagFirstChr == '!'){ //TODO: implement if/while? for?
				endTag[0] = tagFirstChr;
				int endIdx = htmlSource.find(endTag, startIdx);
				if (endIdx == (int)string::npos){
					BOOST_THROW_EXCEPTION(serverError() << stringInfoFromFormat("Error: python tag started at position %1% doesn't end.", startIdx));
				}
				
				string pythonCode = string(it + 2, htmlSource.begin() + endIdx);
				
				if (tagFirstChr == '@'){
					string evalResult = pythonEval(pythonCode);
					responseData += evalResult;
				}
				else{
					pythonRun(pythonCode);
				}
				
				it = htmlSource.begin() + endIdx + 2;
			}
		}
		
		oldIt = it;
		it = find(oldIt, htmlSource.end(), '<');
	}
	
	responseData += string(oldIt, it);
	map<string, string> headersMap  = pythonGetGlobalMap("resp_headers");
	unordered_map<string, string> headers(headersMap.begin(), headersMap.end());
	Response result(1, 1, 200, headers, responseData);
	addDefaultHeaders(result);
	return result;
}


void Server::addDefaultHeaders(Response& response){
	if (!response.headerExists("Content-Type")){
		response.setHeader("Content-Type", "text/html; charset=ISO-8859-8");
	}
	response.setHeader("Connection", "close");
}


string readFromFile(string filename);

string getHtmlSource(string filename){
	//TODO: some caching maybe?
	
	return readFromFile(filename);
}


string readFromFile(string filename){
	ifstream fileIn(filename, ios::in | ios::binary);
	
	if (!fileIn){
		BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Error: File not found: %1%", filename));
	}
	
	ostringstream fileData;
	fileData << fileIn.rdbuf();
	fileIn.close();
	
	return fileData.str();
}
