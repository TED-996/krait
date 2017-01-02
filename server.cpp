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
#include"dbg.h"
#include "path.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;


Server::Server(string serverRoot, int port, LoggerIn infoLogger, LoggerIn errLogger)
	: infoLogger(infoLogger), errLogger(errLogger) {
	this->serverRoot = path(serverRoot);
	this->routes = getRoutesFromFile((this->serverRoot / ".config" / "routes.xml").string());
	
	DBG("routes got");
	
	this->serverSocket = getServerSocket(port, false);
	
	DBG("server socket got");
	
	pythonInit(serverRoot);
	
	infoLogger.log("Server initialized.");
	
	loadContentTypeList();
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
	closeSocket(clientSocket);
	
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
		DBG_FMT("URL is %1%", clientRequest.getUrl());
		
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
		
		DBG_FMT("Response object for client on URL %1% done.", clientRequest.getUrl());
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
	DBG("AAND sent and socket closed.");
	
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


string getEndTag(string htmlCode, int startIdx);
string pythonPrepareStr(string pyCode);
string htmlEscape (string htmlCode);

Response Server::getResponseFromSource(string filename, Request& request){
	//HTML:	<@ @> => echo
	//		<! !> => run
	//		<@! @!> => run, don't escape
	filename = expandFilename(filename);
	
	string htmlSource = getHtmlSource(filename);
	
	if (!boost::algorithm::ends_with(filename, ".html")){
		Response result(1, 1, 200, unordered_map<string, string>{
			{"Content-Type", getContentType(filename)}
		}, htmlSource);
		addDefaultHeaders(result);
		return result;
	}
	else{
		//DBG("Html source got");
		
		auto oldIt = htmlSource.begin();
		auto it = find(oldIt, htmlSource.end(), '<');
		string responseData;
		responseData.reserve(htmlSource.length());
		
		while(it != htmlSource.end()){
			responseData += string(oldIt, it);
			
			if (it != htmlSource.end() && it + 1 != htmlSource.end()){
				//DBG_FMT("in tag; idx= %1%", it - htmlSource.begin());
				int startIdx = it - htmlSource.begin();
				string endTag = getEndTag(htmlSource, startIdx);
				if ( endTag == "@>" || endTag == "@!>" || endTag == "!>"){ //TODO: implement if/while? for?
					int endIdx = htmlSource.find(endTag, startIdx);
					if (endIdx == (int)string::npos){
						BOOST_THROW_EXCEPTION(serverError() << stringInfoFromFormat("Error: python tag started at position %1% doesn't end.", startIdx));
					}
					
					string pythonCode = pythonPrepareStr(string(it + endTag.length(), htmlSource.begin() + endIdx));
					
					if (endTag == "@>"){
						string evalResult = pythonEval(pythonCode);
						responseData += htmlEscape (evalResult);
					}
					else if (endTag == "@!>"){
						string evalResult = pythonEval(pythonCode);
						responseData += evalResult;
					}
					else{
						pythonRun(pythonCode);
					}
					
					it = htmlSource.begin() + endIdx + endTag.length();
				}
			}
			
			oldIt = it;
			it = find(oldIt + 1, htmlSource.end(), '<');
		}
		
		responseData += string(oldIt, it);
		map<string, string> headersMap  = pythonGetGlobalMap("resp_headers");
		unordered_map<string, string> headers(headersMap.begin(), headersMap.end());
		Response result(1, 1, 200, headers, responseData);
		addDefaultHeaders(result);
		return result;
	}
}


string Server::expandFilename(string filename){
	if (filesystem::is_directory(filename)){
		DBG("Converting to directory automatically");
		filename = (filesystem::path(filename) / "index.html").string();
	}
	if (pathBlocked(filename)){
		BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Error: File not found: %1%", filename));
	}
	if (!filesystem::exists(filename) && filesystem::exists(filename + ".html")){
		DBG("Adding .html automatically");
		filename += ".html";
	}
	return filename;
}


string readFromFile(string filename);

string Server::getHtmlSource(string filename){
	//TODO: some caching maybe?
	
	return readFromFile(filename);
}


bool Server::pathBlocked(string filename){
	boost::filesystem::path filePath(filename);
	for (auto& part : filePath){
		if (part.c_str()[0] == '.'){
			DBG_FMT("BANNED because of part %1%", part.c_str());
			return true;
		}
	}
	return false;
}


string readFromFile(string filename){
	std::ifstream fileIn(filename, ios::in | ios::binary);
	
	if (!fileIn){
		BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Error: File not found: %1%", filename));
	}
	
	ostringstream fileData;
	fileData << fileIn.rdbuf();
	fileIn.close();
	
	return fileData.str();
}


string getEndTag(string htmlCode, int startIdx){
	int htmlLen = htmlCode.length();
	if (htmlCode[startIdx] != '<'){
		BOOST_THROW_EXCEPTION(serverError() << stringInfoFromFormat("Error: tried to find closing tag, but this doesn't start with a '<', but a '%1%'", htmlCode[startIdx]));
	}

	int idx = startIdx + 1;
	while(idx < htmlLen && !isspace(htmlCode[idx]) && htmlCode[idx] != '>' && htmlCode[idx] != '/'){
		idx++;
	}
	
	string endTag = htmlCode.substr(startIdx + 1, idx - (startIdx + 1)) + '>';
	//DBG(endTag);
	return endTag;
}


//Dedent string
string pythonPrepareStr(string pyCode){
	string result = pyCode;
	//First remove first line if whitespace.
	
	unsigned int idx;
	bool foundNewline = false;
	for (idx = 0; idx < result.length(); idx++){
		if (foundNewline && result[idx] != '\r' && result[idx] != '\n'){
			break;
		}
		if (!isspace(result[idx])){
			idx = 0;
			break;
		}
		if ( result[idx] == '\n'){
			foundNewline = true;
		}
	}
	unsigned int codeStartIdx = idx;
	
	if (codeStartIdx == result.length()){
		return "";
	}
	
	char indentChr = result[codeStartIdx];
	for (idx = codeStartIdx; idx < result.length(); idx++){
		if ( result[idx] != indentChr){
			break;
		}
	}
	int indentRemove = idx - codeStartIdx;
	if (indentRemove == 0){
		return pyCode;
	}
	//DBG_FMT("Indent is %1%", indentRemove);
	
	for (idx = codeStartIdx; idx < result.length(); idx++){
		int idx2;
		for (idx2 = 0; idx2 < indentRemove; idx2++){
			if (result[idx + idx2] == '\r'){
				continue;
			}
			if (result[idx + idx2] == '\n'){
				//DBG("Premature break in dedent");
				break;
			}
			if ( result[idx + idx2] != indentChr){
				BOOST_THROW_EXCEPTION(serverError() << stringInfoFromFormat("Unexpected character '%c' (\\x%02x) at pos %d in indent before Python code; expected '%c' (\\x%02x)",
																			result[idx + idx2], (int) result[idx + idx2], idx + idx2, indentChr, (int)indentChr));
			}
		}
		result.erase(idx, idx2);
		for (; idx < result.length(); idx++){
			if ( result[idx] == '\n'){
				break;
			}
		}
	}
	
	return result;
}


string htmlEscape (string htmlCode){
	string result;
	
	const char* replacements[256];
	memzero(replacements);
	replacements[(int)'&'] = "&amp;";
	replacements[(int)'<'] = "&lt";
	replacements[(int)'>'] = "&gt";
	replacements[(int)'"'] = "&quot;";
	replacements[(int)'\''] = "&#39";
	
	unsigned int oldIdx = 0;
	for (unsigned int idx = 0; idx < htmlCode.length(); idx++){
		if (replacements[(int)htmlCode[idx]] != NULL){
			result += htmlCode.substr(oldIdx, idx - oldIdx) + replacements[(int)htmlCode[idx]];
			oldIdx = idx + 1;
		}
	}
	
	return result + htmlCode.substr(oldIdx, htmlCode.length() - oldIdx);
}


void Server::addDefaultHeaders(Response& response){
	if (!response.headerExists("Content-Type")){
		response.setHeader("Content-Type", "text/html; charset=ISO-8859-8");
	}
	response.setHeader("Connection", "close");
}


string Server::getContentType(string filename){
	filesystem::path filePath(filename);
	string extension = filePath.extension().string();

	DBG_FMT("Extension: %1%", extension);
	
	auto it = contentTypeByExtension.find(extension);
	if (it == contentTypeByExtension.end()){
		return "application/octet-stream";
	}
	else{
		return it->second;
	}
	
}


void Server::loadContentTypeList() {
	filesystem::path contentFilePath = getExecRoot() / "globals" / "mime.types";
	std::ifstream mimeFile(contentFilePath.string());
	
	std::string line;
	while(getline(mimeFile, line)){
		vector<string> lineItems;
		algorithm::split(lineItems, line, is_any_of(" \t\r\n"), token_compress_on);
		
		if (lineItems.size() < 2 || lineItems[0][0] == '#'){
			continue;
		}
		
		string mimeType = lineItems[0];
		for (unsigned int i = 1; i < lineItems.size(); i++){
			contentTypeByExtension["." + lineItems[i]] = mimeType;
		}
	}
}
