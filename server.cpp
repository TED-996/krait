#include<unistd.h>
#include<sys/wait.h>
#include<signal.h>
#include<algorithm>
#include<fstream>
#include<cctype>
#include<vector>
#include<boost/filesystem.hpp>
#include<boost/algorithm/string.hpp>
#include"utils.h"
#include"server.h"
#include"except.h"
#include"pythonWorker.h"
#include "path.h"


#include"dbg.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::algorithm;


Server::Server(string serverRoot, int port, LoggerIn infoLogger, LoggerIn errLogger)
	: infoLogger(infoLogger), errLogger(errLogger) {
	this->serverRoot = path(serverRoot);
	
	filesystem::path routesFilename = this->serverRoot / ".config" / "routes.xml";
	if (filesystem::exists(routesFilename)){
		this->routes = getRoutesFromFile(routesFilename.string());
	}
	else{
		infoLogger.log("No routes file found; default used (all GETs to default target;) create a file named \".config/routes.xml\" in the server root directory.");
		this->routes = getDefaultRoutes();
	}
	

	DBG("routes got");

	this->serverSocket = getServerSocket(port, false);

	DBG("server socket got");

	pythonInit(serverRoot);

	infoLogger.log("Server initialized.");

	loadContentTypeList();
}


Server::~Server() {
	//TODO: some signaling?
	close(serverSocket);
	while (pids.size() != 0) {
		tryWaitFinishedForks();
		sleep(1);
	}
	infoLogger.log("Server destructed.");
}


void Server::killChildren() {
	for (int pid : pids) {
		kill((pid_t)pid, SIGTERM);
	}
	infoLogger.log("Chilren killed.");
}


void Server::runServer() {
	setSocketListen(this->serverSocket);
	infoLogger.log("Server running");

	while (true) {
		tryAcceptConnection();
		tryWaitFinishedForks();
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
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Error: could not fork()! Is the system out of resources?") <<
		                      errcodeInfo(errno));
	}
	if (pid == 0) {
		closeSocket(serverSocket);
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

	int status;
	pid_t pid;
	while ((pid = waitpid(-1, &status, WNOHANG) != 0)) {
		if (WIFEXITED(status) || WIFSIGNALED(status) || WIFSTOPPED(status)) {
			DBG_FMT("[Parent] Child exited with status %1%", WEXITSTATUS(status));
			pids.erase((int)pid);
		}
	}
}


string replaceParams(string target, map<string, string> params);

void Server::serveClientStart(int clientSocket) {
	infoLogger.log("Serving a new client");
	//TODO: handle SIGTERM in some way...
	//TODO: handle execptions (return 5xx + log)
	Response resp(1, 1, 500, unordered_map<string, string>(), string());

	bool isHead = false;

	try {
		Request clientRequest = getRequestFromSocket(clientSocket);
		//DBG_FMT("URL is %1%", clientRequest.getUrl());

		map<string, string> params;
		Route route = getRouteMatch(routes, clientRequest.getVerb(), clientRequest.getUrl(), params);

		if (clientRequest.getVerb() == HttpVerb::HEAD) {
			isHead = true;
			clientRequest.setVerb(HttpVerb::GET);
		}

		string sourceFile;
		if (route.isDefaultTarget()) {
			sourceFile = getFilenameFromTarget (clientRequest.getUrl());
		}
		else {
			string targetReplaced = replaceParams(route.getTarget(), params);
			sourceFile = getFilenameFromTarget (route.getTarget());
		}

		pythonSetGlobalRequest("request", clientRequest);
		pythonSetGlobal("url_params", params);
		pythonSetGlobal("resp_headers", map<string, string>());

		resp = getResponseFromSource(sourceFile, clientRequest);

		DBG_FMT("Response object for client on URL %1% done.", clientRequest.getUrl());
	}
	catch (notFoundError&) {
		resp = Response(1, 1, 404, unordered_map<string, string>(), string());
	}
	catch (rootException& ex) {
		resp = Response(1, 1, 500, unordered_map<string, string>(), string());
		errLogger.log(formatString("Error serving client: %1%", ex.what()));
	}

	if (isHead) {
		resp.setBody(string());
	}

	respondWithObject(clientSocket, resp);

	closeSocket(clientSocket);

	//TODO: recycle connection if necessary
	DBG("AAND sent and socket closed.");

	exit(0);
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

bool canContainPython(string filename);

Response Server::getResponseFromSource(string filename, Request& request) {
	//TODO: unused request?
	//HTML: <@ @> => echo
	//      <! !> => run
	//      <@! @!> => run, don't escape
	//DBG_FMT("Filename is %1%", filename);
	filename = expandFilename(filename);
	
	if (!filesystem::exists(filename)){
		BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Error: File not found: %1%", filename));
	}

	if (!canContainPython(filename)) {
		Response result(1, 1, 200, unordered_map<string, string> {
			{"Content-Type", getContentType(filename)}
		}, getRawFile(filename));
		
		addDefaultHeaders(result);
		return result;
	}
	else {
		string pymlResult = getPymlResult (filename);

		map<string, string> headersMap  = pythonGetGlobalMap("resp_headers");
		unordered_map<string, string> headers(headersMap.begin(), headersMap.end());
		Response result(1, 1, 200, headers, pymlResult);
		
		addDefaultHeaders(result);
		return result;
	}
}


bool canContainPython(std::string filename){
	return ends_with(filename, ".html") || ends_with(filename, ".htm") ||
		ends_with(filename, ".pyml");
}


string Server::expandFilename(string filename) {
	if (filesystem::is_directory(filename)) {
		DBG("Converting to directory automatically");
		filename = (filesystem::path(filename) / "index").string(); //Not index.html; taking care below about it
	}
	if (pathBlocked(filename)) {
		BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Error: File not found: %1%", filename));
	}

	if (!filesystem::exists(filename)){
		if (filesystem::exists(filename + ".html")){
			DBG("Adding .html automatically");
			filename += ".html";
		}
		if (filesystem::exists(filename + ".htm")){
			DBG("Adding .htm automatically");
			filename += ".htm";
		}
		if (filesystem::exists(filename + ".pyml")){
			DBG("Adding .pyml automatically");
			filename += ".pyml";
		}
		
		if (path(filename).extension() == ".html" && filesystem::exists(filesystem::change_extension(filename, ".htm"))){
			filename = filesystem::change_extension(filename, "htm").string();
			DBG("Changing extension to .htm");
		}
		if (path(filename).extension() == ".htm" && filesystem::exists(filesystem::change_extension(filename, ".html"))){
			filename = filesystem::change_extension(filename, "html").string();
			DBG("Changing extension to .html");
		}
	}
	return filename;
}


string readFromFile(string filename);

string Server::getPymlResult (string filename) {
	//DBG("Reading pyml cache");
	const auto it = pymlCache.find(filename);

	if (it == pymlCache.end()){
		const PymlFile& file = addPymlToCache(filename);
		return file.runPyml();
	}
	if (last_write_time(filename) != it->second.first){
		pymlPool.destroy(it->second.second);
		pymlCache.erase(it);
		const PymlFile& file = addPymlToCache(filename);
		return file.runPyml();
	}
	
	return it->second.second->runPyml();
}


const PymlFile& Server::addPymlToCache(string filename) { //TODO: caching kind of flawed, necessary in parent process, not child! Send filename through pipe?
	//DBG("Adding new item to cache");
	time_t time = last_write_time(filename);
	PymlFile* pymlFile = pymlPool.construct(readFromFile(filename));
	pymlCache[filename] = make_pair(time, pymlFile);
	return *pymlFile;
}


string Server::getRawFile(string filename) {
	//DBG("Reading raw cache");

	const auto it = rawFileCache.find(filename);

	if (it == rawFileCache.end()){
		return addRawFileToCache(filename);
	}
	if (last_write_time(filename) != it->second.first){
		rawFileCache.erase(it);
		return addRawFileToCache(filename);
	}
	
	return it->second.second;
}


string& Server::addRawFileToCache(string filename){
	time_t time = last_write_time(filename);
	
	string result = readFromFile(filename);
	rawFileCache[filename] = make_pair(time, result);
	
	return rawFileCache[filename].second;
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


string readFromFile(string filename) {
	std::ifstream fileIn(filename, ios::in | ios::binary);

	if (!fileIn) {
		BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Error: File not found: %1%", filename));
	}

	ostringstream fileData;
	fileData << fileIn.rdbuf();
	fileIn.close();

	return fileData.str();
}


void Server::addDefaultHeaders(Response& response) {
	if (!response.headerExists("Content-Type")) {
		response.setHeader("Content-Type", "text/html; charset=ISO-8859-8");
	}
	response.setHeader("Connection", "close");
}


string Server::getContentType(string filename) {
	filesystem::path filePath(filename);
	string extension = filePath.extension().string();
	if (extension == ".pyml"){
		extension = filePath.stem().extension().string();
	}

	DBG_FMT("Extension: %1%", extension);

	auto it = contentTypeByExtension.find(extension);
	if (it == contentTypeByExtension.end()) {
		return "application/octet-stream";
	}
	else {
		return it->second;
	}

}


void Server::loadContentTypeList() {
	filesystem::path contentFilePath = getExecRoot() / "globals" / "mime.types";
	std::ifstream mimeFile(contentFilePath.string());

	std::string line;
	while (getline(mimeFile, line)) {
		vector<string> lineItems;
		algorithm::split(lineItems, line, is_any_of(" \t\r\n"), token_compress_on);

		if (lineItems.size() < 2 || lineItems[0][0] == '#') {
			continue;
		}

		string mimeType = lineItems[0];
		for (unsigned int i = 1; i < lineItems.size(); i++) {
			contentTypeByExtension["." + lineItems[i]] = mimeType;
		}
	}
}
