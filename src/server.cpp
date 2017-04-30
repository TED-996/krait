#include<unistd.h>
#include<sys/wait.h>
#include<signal.h>
#include<vector>
#include<ctime>
#include<boost/filesystem.hpp>
#include<boost/algorithm/string/trim.hpp>
#include<boost/algorithm/string/predicate.hpp>
#include<functional>
#include<string.h>
#include<memory>
#include"utils.h"
#include"server.h"
#include"except.h"
#include "pythonModule.h"
#include"path.h"
#include"logger.h"
#include"pymlIterator.h"
#include"rawPymlParser.h"
#include"v2PymlParser.h"

#include"dbg.h"
#include "rawPythonPymlParser.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;


unordered_set<int> Server::pids;
int Server::socketToClose;
bool Server::clientWaitingResponse;



Server::Server(string serverRoot, int port) :
	cacheController((path(serverRoot) / ".config" / "cache-private.cfg").string(),
					(path(serverRoot) / ".config" / "cache-public.cfg").string(),
					(path(serverRoot) / ".config" / "cache-nostore.cfg").string(),
					(path(serverRoot) / ".config" / "cache-longterm.cfg").string()),
	serverCache(
			std::bind(&Server::constructPymlFromFilename,
			     this,
			     std::placeholders::_1,
			     std::placeholders::_2,
			     std::placeholders::_3),
			std::bind(&Server::onServerCacheMiss, this, std::placeholders::_1)) {
	this->serverRoot = path(serverRoot);
	socketToClose = -1;
	clientWaitingResponse = false;
	
	filesystem::path routesFilename = this->serverRoot / ".config" / "routes.json";
	if (filesystem::exists(routesFilename)){
		this->routes = Route::getRoutesFromFile(routesFilename.string());
	}
	else{
		Loggers::logInfo("No routes file found; default used (all GETs to default target;) create a file named \".config/routes.xml\" in the server root directory.");
		this->routes = Route::getDefaultRoutes();
	}
	
	DBG("routes got");

	this->serverSocket = getServerSocket(port, false, true);
	socketToClose = this->serverSocket;

	DBG("server socket got");

	PythonModule::initModules(serverRoot);

	DBG("python initialized");

	loadContentTypeList();

	initSignals();

	Loggers::logInfo(formatString("Server initialized on port %1%", port));

	stdinDisconnected = fdClosed(0);
}

void Server::initSignals(){
	struct sigaction termAction;
	struct sigaction intAction;

	memzero(termAction);
	memzero(intAction);

	termAction.sa_handler = &Server::signalKillRequested;
	intAction.sa_handler = &Server::signalStopRequested;

	sigaction(SIGINT, &intAction, NULL);
	sigaction(SIGUSR1, &intAction, NULL);
	sigaction(SIGTERM, &termAction, NULL);
}


void Server::signalStopRequested(int sig){
	Loggers::logInfo(formatString("Stop requested for process %1%", getpid()));
	clientWaitingResponse = false;
	if (clientWaitingResponse && socketToClose != -1){
		respondWithObject(socketToClose,  Response(1, 1, 500, unordered_map<string, string>(), "<html><body><h1>500 Internal Server Error</h1></body></html>", true));
	}
	if (socketToClose != -1){
		close(socketToClose);
		socketToClose = -1;
	}

	for (int pid : pids){
		kill((pid_t)pid, SIGUSR1);
	}

	while (pids.size() != 0) {
		tryWaitFinishedForks();
		sleep(1);
		DBG("not closed...");
	}

	Loggers::logInfo(formatString("Process %1% closed.", getpid()));	
	exit(0);
}


void Server::signalKillRequested(int sig){
	Loggers::logInfo(formatString("Kill requested for process %1%", getpid()));
	//hard close

	if (socketToClose != -1){
		close(socketToClose);
		socketToClose = -1;
	}

	for (int pid : pids){
		kill((pid_t)pid, SIGTERM);
	}

	Loggers::logInfo(formatString("Process %1% killed.", getpid()));
	exit(0);
}


Server::~Server() {
	if (socketToClose != -1){
		close(socketToClose);
	}
	//Try soft close
	for (int pid : pids){
		kill((pid_t)pid, SIGUSR1);
	}

	while (pids.size() != 0) {
		tryWaitFinishedForks();
		sleep(1);
	}
	Loggers::logInfo("Server destructed.");
}


void Server::killChildren() {
	//First try to get them to shut down graciously.
	for (int pid : pids){
		kill((pid_t)pid, SIGUSR1);
	}
	sleep(1);
	for (int pid : pids) {
		kill((pid_t)pid, SIGTERM);
	}
	Loggers::logInfo("Chilren killed.");
}


void Server::runServer() {
	setSocketListen(this->serverSocket);
	Loggers::logInfo("Server listening");

	while (true) {
		tryAcceptConnection();
		tryWaitFinishedForks();
		updateParentCaches();
		tryCheckStdinClosed();
	}
}


void Server::tryAcceptConnection() {
	const int timeout = 100;
	int clientSocket = getNewClient(serverSocket, timeout);

	if (clientSocket == -1) {
		return;
	}

	pid_t pid = fork();
	if (pid == -1) {
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("fork(): creating process to serve socket. Is the system out of resources?") <<
		                      errcodeInfoDef());
	}
	if (pid == 0) {
		pids.clear();
		socketToClose = clientSocket;
		
		closeSocket(serverSocket);
		cacheRequestPipe.closeRead();

		serveClientStart(clientSocket);//TODO: connect stderr?
		exit(255); //The function above should call exit()!
	}
	closeSocket(clientSocket);

	pids.insert((int)pid);
}


void Server::tryWaitFinishedForks() {
	if (pids.size() == 0) {
		return;
	}
	//DBG_FMT("there are %1% pids", pids.size());

	int status;
	pid_t pid;
	while (pids.size() != 0 && (pid = waitpid(-1, &status, WNOHANG)) != 0) {
		if (pid == -1){
			BOOST_THROW_EXCEPTION(syscallError() << stringInfo("waitpid: waiting for child") << errcodeInfoDef());
		}
		if (WIFEXITED(status) || WIFSIGNALED(status)) {
			DBG_FMT("[Parent] Child %1% exited with status %2%", pid, WEXITSTATUS(status));
			
			auto it = pids.find((int)pid);
			if (it != pids.end()){
				pids.erase(it);
			}
		}
	}
}

void Server::tryCheckStdinClosed(){
	if (!stdinDisconnected && fdClosed(0)){
		kill(getpid(), SIGUSR1);
	}
}

string replaceParams(string target, map<string, string> params);

void Server::serveClientStart(int clientSocket) {
	Loggers::logInfo("Serving a new client");
	bool isHead = false;
	keepAliveTimeoutSec = maxKeepAliveSec;

	try{
		while(true){
			optional<Request> requestOpt = getRequestFromSocket(clientSocket, keepAliveTimeoutSec * 1000);
			clientWaitingResponse = true;
			Loggers::logInfo(formatString("request got"));
			if (!requestOpt){
				Loggers::logInfo(formatString("Requests finished."));
				break;
			}
			Request& request = *requestOpt;
			
			Loggers::logInfo(formatString("Request URL is %1%", request.getUrl()));
			if (request.getVerb() == HttpVerb::HEAD) {
				isHead = true;
			}
			if (request.headerExists("If-Modified-Since")){
				Loggers::logInfo(formatString("Client tried If-Modified-Since with date %1%", *request.getHeader("If-Modified-Since")));
			}

			keepAliveTimeoutSec = min(maxKeepAliveSec, request.getKeepAliveTimeout());
			keepAlive = request.isKeepAlive() && keepAliveTimeoutSec != 0;
			
			pid_t childPid = fork();
			if (childPid == 0){
				pids.clear(); 

				serveRequest(clientSocket, request);
				clientWaitingResponse = false;
				
				Loggers::logInfo("Serving a request finished.");
				close(clientSocket);
				exit(0);
			}
			else{
				clientWaitingResponse = false;
				pids.insert((int)childPid);
				if (waitpid(childPid, NULL, 0) != childPid){
					BOOST_THROW_EXCEPTION(syscallError() << stringInfo("waitpid(): waiting for request responder process") << errcodeInfoDef());
				}
				pids.erase((int)childPid);
				Loggers::logInfo("Rejoined with forked subfork.");
			}
			
			if (!keepAlive){
				break;
			}
		}
	}
	catch (rootException& ex) {
		if (isHead){
			respondWithObject(clientSocket, Response(1, 1, 500, unordered_map<string, string>(), "", true));
		}
		else{
			respondWithObject(clientSocket, Response(1, 1, 500, unordered_map<string, string>(),
							  "<html><body><h1>500 Internal Server Error</h1></body></html>", true));
		}
		Loggers::logErr(formatString("Error serving client: %1%", ex.what()));
	}
	
	
	close(clientSocket);

	exit(0);
}

void Server::serveRequest(int clientSocket, Request& request) {
	Response resp(1, 1, 500, unordered_map<string, string>(), string(), true);

	bool isHead = false;

	try {
		if (request.getVerb() == HttpVerb::HEAD) {
			isHead = true;
			request.setVerb(HttpVerb::GET);
		}

		map<string, string> params;
		const Route& route = Route::getRouteMatch(routes, request.getVerb(), request.getUrl(), params);

		string targetReplaced = replaceParams(route.getTarget(request.getUrl()), params);
		string sourceFile = getFilenameFromTarget(targetReplaced);

		PythonModule::krait.setGlobalRequest("request", request);
		PythonModule::krait.setGlobal("url_params", params);
		PythonModule::krait.setGlobal("extra_headers", map<string, string>());

		resp = getResponseFromSource(sourceFile, request);		
		//DBG_FMT("Response object for client on URL %1% done.", request.getUrl());
	}
	catch (notFoundError& err) {
		DBG_FMT("notFound: %1%", err.what());
		resp = Response(1, 1, 404, unordered_map<string, string>(), "<html><body><h1>404 Not Found</h1></body></html>", true);
	}
	catch (rootException& ex) {
		resp = Response(1, 1, 500, unordered_map<string, string>(), "<html><body><h1>500 Internal Server Error</h1></body></html>", true);
		Loggers::logErr(formatString("Error serving client: %1%", ex.what()));
	}

	if (isHead) {
		resp.setBody(string(), false);
	}
	
	resp.setConnClose(!keepAlive);

	respondWithObjectRef(clientSocket, resp);
}



std::string Server::getFilenameFromTarget (std::string target) {
	if (target[0] == '/') {
		return (serverRoot / target.substr(1)).string();
	}
	else {
		return (serverRoot / target).string();
	}
}


string replaceParams(string target, map<string, string> params) {
	auto it = target.begin();
	auto oldIt = it;
	string result;

	while ((it = find(it, target.end(), '{')) != target.end()) {
		result += string(oldIt, it);
		auto endIt = find(it, target.end(), '}');
		if (endIt == target.end()) {
			BOOST_THROW_EXCEPTION(routeError() << stringInfoFromFormat("Error: unmatched paranthesis in route target %1%", target));
		}

		string paramName(it + 1, endIt);
		auto paramFound = params.find(paramName);
		if (paramFound == params.end()) {
			BOOST_THROW_EXCEPTION(routeError() << stringInfoFromFormat("Error: parameter name %1% in route target %2% not found",
			                      paramName, target));
		}
		result += paramFound->second;
		it++;
		oldIt = it;
	}
	result += string(oldIt, it);

	return result;
}


Response Server::getResponseFromSource(string filename, Request& request) {
	//TODO: unused request?
	filename = expandFilename(filename);
	
	if (!filesystem::exists(filename)){
		BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Error: File not found: %1%", filename));
	}

	Response result(1, 1, 500, unordered_map<string, string>(), "", true);	

	bool isDynamic = getPymlIsDynamic(filename);
	CacheController::CachePragma cachePragma = cacheController.getCacheControl(
			filesystem::relative(filename, serverRoot).string(), !isDynamic);

	string etag;
	if (request.headerExists("if-none-match")){
		etag = *request.getHeader("if-none-match");
		etag = etag.substr(1, etag.length() - 2);
	}
	if (cachePragma.isStore && serverCache.checkCacheTag(filename, etag)){
		result = Response(1, 1, 304, unordered_map<string, string>(), "", false);
	}
	else{
		IteratorResult pymlResult = getPymlResultRequestCache(filename);

		map<string, string> headersMap = PythonModule::krait.getGlobalMap("extra_headers");
		unordered_map<string, string> headers(headersMap.begin(), headersMap.end());
		

		if (!PythonModule::krait.checkIsNone("response")){
			result = Response(PythonModule::krait.eval("str(response)"));
			for (const auto& header : headers){
				result.addHeader(header.first, header.second);
			}
		}
		else {
			result = Response(1, 1, 200, headers, pymlResult, false);
		}
	}

	addStandardCacheHeaders(result, filename, cachePragma);


	addDefaultHeaders(result, filename, request);
	
	return result;
}


bool Server::canContainPython(std::string filename){
	return ends_with(filename, ".html") || ends_with(filename, ".htm") ||
		ends_with(filename, ".pyml");
}


string Server::expandFilename(string filename) {
	if (filesystem::is_directory(filename)) {
		//DBG("Converting to directory automatically");
		filename = (filesystem::path(filename) / "index").string(); //Not index.html; taking care below about it
	}
	if (pathBlocked(filename)) {
		DBG_FMT("Blocking path %1%", filename);
		BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Error: File not found: %1%", filename));
	}

	if (!filesystem::exists(filename)){
		if (filesystem::exists(filename + ".html")){
			//DBG("Adding .html automatically");
			filename += ".html";
		}
		else if (filesystem::exists(filename + ".htm")){
			//DBG("Adding .htm automatically");
			filename += ".htm";
		}
		else if (filesystem::exists(filename + ".pyml")){
			//DBG("Adding .pyml automatically");
			filename += ".pyml";
		}
		else if (filesystem::exists(filename + ".py")){
			filename += ".py";
		}
		
		else if (path(filename).extension() == ".html" &&
				filesystem::exists(filesystem::change_extension(filename, ".htm"))){
			filename = filesystem::change_extension(filename, "htm").string();
			//DBG("Changing extension to .htm");
		}
		else if (path(filename).extension() == ".htm" &&
				filesystem::exists(filesystem::change_extension(filename, ".html"))){
			filename = filesystem::change_extension(filename, "html").string();
			//DBG("Changing extension to .html");
		}
	}
	return filename;
}



IteratorResult Server::getPymlResultRequestCache(string filename) {
	//DBG("Reading pyml cache");
	interpretCacheRequest = true;
	const IPymlFile* pymlFile = serverCache.get(filename);
	return IteratorResult(PymlIterator(pymlFile->getRootItem()));
}


bool Server::getPymlIsDynamic(string filename){
	interpretCacheRequest = true;
	const IPymlFile* pymlFile = serverCache.get(filename);
	return pymlFile->isDynamic();
}


PymlFile* Server::constructPymlFromFilename(std::string filename, boost::object_pool<PymlFile>& pool, char* tagDest){
	DBG_FMT("constructFromFilename(%1%)", filename);
	string source = readFromFile(filename);
	generateTagFromStat(filename, tagDest);
	unique_ptr<IPymlParser> parser;
	if (canContainPython(filename)) {
		DBG("choosing v2 pyml parser");
		parser = unique_ptr<IPymlParser>(new V2PymlParser(serverCache));
	}
	else if (ends_with(filename, ".py")){
		parser = unique_ptr<IPymlParser>(new RawPythonPymlParser(serverCache));
	}
	else{
		parser = unique_ptr<IPymlParser>(new RawPymlParser());
	}
	return pool.construct(source.begin(), source.end(), parser);
}

void Server::onServerCacheMiss(std::string filename){
	if (interpretCacheRequest){
		cacheRequestPipe.pipeWrite(filename);
		Loggers::logInfo(formatString("Cache miss on url %1%", filename));	
	}
}

void Server::updateParentCaches() {
	while(cacheRequestPipe.pipeAvailable()){
		string filename = cacheRequestPipe.pipeRead();
		DBG("next cache add is in parent");

		interpretCacheRequest = false;
		serverCache.get(filename);
		interpretCacheRequest = true;
	}
}


bool Server::pathBlocked(string filename) {
	boost::filesystem::path filePath(filename);
	for (auto & part : filePath) {
		if (part.c_str()[0] == '.') {
			DBG_FMT("BANNED because of part %1%", part.c_str());
			return true;
		}
	}
	return false;
}

void Server::addDefaultHeaders(Response& response, string filename, Request& request) {
	if (!response.headerExists("Content-Type")) {
		response.setHeader("Content-Type", getContentType(filename));
	}
	std::time_t timeVal = std::time(NULL);
	if (timeVal != -1 && !response.headerExists("Date")) {
		response.setHeader("Date", unixTimeToString(timeVal));
	}
}


string Server::getContentType(string filename) {
	string extension;

	if (PythonModule::krait.checkIsNone("content_type")) {
		DBG("No content_type set");
		filesystem::path filePath(filename);
		extension = filePath.extension().string();
		if (extension == ".pyml") {
			extension = filePath.stem().extension().string();
		}
	}
	else{
		string varContentType = PythonModule::krait.getGlobalStr("content_type");
		DBG_FMT("content_type set to %1%", varContentType);

		if (!starts_with(varContentType, "ext/")){
			return varContentType;
		}
		else{
			extension = varContentType.substr(3); //strlen("ext")
			extension[0] = '.'; // /extension to .extension
		}
	}

	//DBG_FMT("Extension: %1%", extension);

	auto it = contentTypeByExtension.find(extension);
	if (it == contentTypeByExtension.end()) {
		return "application/octet-stream";
	}
	else {
		return it->second;
	}
}


void Server::addStandardCacheHeaders(Response& response, string filename, CacheController::CachePragma pragma){
	response.setHeader("cache-control", cacheController.getValueFromPragma(pragma));
	
	if (pragma.isStore){
		response.addHeader("etag", "\"" + serverCache.getCacheTag(filename) + "\"");
	}
}


void Server::loadContentTypeList() {
	filesystem::path contentFilePath = getExecRoot() / "globals" / "mime.types";
	std::ifstream mimeFile(contentFilePath.string());

	char line[1024];
	string mimeType;

	while (mimeFile.getline(line, 1023)) {
		if (line[0] == '\0' || line[0] == '#'){
			continue;
		}

		const char* separators = " \t\r\n";
		char* ptr = strtok(line, separators);
		if (ptr == NULL){
			continue;
		}

		mimeType.assign(ptr);
		ptr = strtok(NULL, separators);
		while(ptr != NULL){
			//This SHOULD be fine.
			//Make it an extension (html to .html)
			*(ptr - 1) = '.';
			contentTypeByExtension[string(ptr - 1)] = mimeType;
			
			ptr = strtok(NULL, separators);
		}
		
	}
}
