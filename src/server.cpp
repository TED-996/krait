#include <unistd.h>
#include <ctime>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <functional>
#include <string.h>
#include <memory>
#include "utils.h"
#include "server.h"
#include "except.h"
#include "pythonModule.h"
#include "path.h"
#include "logger.h"
#include "pymlIterator.h"
#include "rawPymlParser.h"
#include "v2PymlParser.h"
#include "websocketsServer.h"
#include "rawPythonPymlParser.h"
#include "signalManager.h"
#include "config.h"

#define DBG_DISABLE
#include"dbg.h"


namespace b = boost;
namespace bf = boost::filesystem;
namespace ba = boost::algorithm;

Server* Server::instance = nullptr;

Server::Server(std::string serverRoot, int port)
	:
	config(),
	cacheController(config),
	serverCache(
		std::bind(&Server::constructPymlFromFilename,
		          this,
		          std::placeholders::_1,
		          std::placeholders::_2,
		          std::placeholders::_3),
		std::bind(&Server::onServerCacheMiss, this, std::placeholders::_1)) {

	if (Server::instance != nullptr) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Multiple Server instances!"));
	}
	Server::instance = this;

	this->serverRoot = bf::path(serverRoot);
	socketToClose = -1;

	DBG("routes got");

	try {
		this->serverSocket = getServerSocket(port, false, true);
	}
	catch (networkError err) {
		Loggers::errLogger.log("Could not get server socket.");
		exit(1);
	}
	socketToClose = this->serverSocket;

	DBG("server socket got");

	try {
		PythonModule::initModules(serverRoot);
	}
	catch (pythonError& err) {
		Loggers::logErr(formatString("Error running init.py: %1%", err.what()));
		exit(1);
	}

	DBG("python initialized");

	loadContentTypeList();

	DBG("mime.types initialized.");

	config.load();
	cacheController.load();

	Loggers::logInfo(formatString("Server initialized on port %1%", port));

	stdinDisconnected = fdClosed(0);
	
	shutdownRequested = false;
}


Server::~Server() {
	if (socketToClose != -1) {
		close(socketToClose);
	}
}

void Server::runServer() {
	try {
		setSocketListen(this->serverSocket);
	}
	catch (networkError err) {
		Loggers::errLogger.log("Could not set server to listen.");
		exit(1);
	}
	Loggers::logInfo("Server listening");

	while (!shutdownRequested) {
		tryAcceptConnection();
		SignalManager::waitStoppedChildren();
		updateParentCaches();
		tryCheckStdinClosed();
	}
}

void Server::requestShutdown() {
	shutdownRequested = true;
}

void Server::cleanup() {
}


void Server::tryAcceptConnection() {
	const int timeout = 100;
	int clientSocket = -1;
	try {
		clientSocket = getNewClient(serverSocket, timeout);
	}
	catch (networkError err) {
		Loggers::errLogger.log("Could not get new client.");
		exit(1);
	}

	if (clientSocket == -1) {
		return;
	}

	pid_t pid = fork();
	if (pid == -1) {
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("fork(): creating process to serve socket. Is the system out of resources?") <<
			errcodeInfoDef());
	}
	if (pid == 0) {
		SignalManager::clearPids();
		socketToClose = clientSocket;

		closeSocket(serverSocket);
		cacheRequestPipe.closeRead();

		serveClientStart(clientSocket);
		exit(255); //The function above should call exit()!
	}
	closeSocket(clientSocket);

	SignalManager::addPid((int)pid);
}


void Server::tryCheckStdinClosed() const {
	if (!stdinDisconnected && fdClosed(0)) {
		raise(SIGUSR1); //TODO: change to SIGUSR2 when proper shutdown is implemented.
	}
}

std::string replaceParams(std::string target, std::map<std::string, std::string> params);

void Server::serveClientStart(int clientSocket) {
	Loggers::logInfo("Serving a new client");
	bool isHead = false;
	keepAliveTimeoutSec = maxKeepAliveSec;

	try {
		while (true) {
			b::optional<Request> requestOpt = getRequestFromSocket(clientSocket, keepAliveTimeoutSec * 1000);
			Loggers::logInfo(formatString("request got"));
			if (!requestOpt) {
				Loggers::logInfo(formatString("Requests finished."));
				break;
			}
			Request& request = *requestOpt;

			Loggers::logInfo(formatString("Request URL is %1%", request.getUrl()));
			if (request.getVerb() == HttpVerb::HEAD) {
				isHead = true;
			}
			if (request.headerExists("If-Modified-Since")) {
				Loggers::logInfo(formatString("Client tried If-Modified-Since with date %1%", *request.getHeader("If-Modified-Since")));
			}

			keepAliveTimeoutSec = std::min(maxKeepAliveSec, request.getKeepAliveTimeout());
			keepAlive = request.isKeepAlive() && keepAliveTimeoutSec != 0 && !request.isUpgrade("websocket");

			pid_t childPid = fork();
			if (childPid == 0) {
				SignalManager::clearPids();

				try {
					if (request.isUpgrade("websocket")) {
						startWebsocketsServer(clientSocket, request);
					}
					else {
						serveRequest(clientSocket, request);
					}

					Loggers::logInfo("Serving a request finished.");
				}
				catch (networkError&) {
					Loggers::errLogger.log("Could not respond to client request.");
					exit(1);
				}
				catch (pythonError& err) {
					Loggers::errLogger.log(formatString("Python error:\n%1%", err.what()));
					exit(1);
				}
				close(clientSocket);
				exit(0);
			}
			else {
				SignalManager::addPid((int)childPid);
				SignalManager::waitChild((int)childPid);
				Loggers::logInfo("Rejoined with forked request server.");
			}

			if (!keepAlive) {
				break;
			}
		}
	}
	catch (networkError&) {
		Loggers::logErr("Client disconnected.");
	}
	catch (rootException& ex) {
		if (isHead) {
			respondWithObject(clientSocket, Response(500, "", true));
		}
		else {
			respondWithObject(clientSocket, Response(500, "<html><body><h1>500 Internal Server Error</h1></body></html>", true));
		}
		Loggers::logErr(formatString("Error serving client: %1%", ex.what()));
	}


	close(clientSocket);

	exit(0);
}

void Server::serveRequest(int clientSocket, Request& request) {
	Response resp(500, "", true);

	bool isHead = false;

	try {
		if (request.getVerb() == HttpVerb::HEAD) {
			isHead = true;
		}
		std::map<std::string, std::string> params;
		const Route& route = Route::getRouteMatch(config.getRoutes(), request.getRouteVerb(), request.getUrl(), params);

		std::string targetReplaced = replaceParams(route.getTarget(request.getUrl()), params);
		std::string sourceFile = getFilenameFromTarget(targetReplaced);

		PythonModule::krait.setGlobalRequest("request", request);
		PythonModule::krait.setGlobal("url_params", params);
		PythonModule::krait.setGlobal("extra_headers", std::multimap<std::string, std::string>());

		resp = getResponseFromSource(sourceFile, request);
	}
	catch (notFoundError& err) {
		DBG_FMT("notFound: %1%", err.what());
		resp = Response(404, "<html><body><h1>404 Not Found</h1></body></html>", true);
	}
	catch (rootException& ex) {
		resp = Response(500, "<html><body><h1>500 Internal Server Error</h1></body></html>", true);
		Loggers::logErr(formatString("Error serving client: %1%", ex.what()));
	}

	if (isHead) {
		resp.setBody(std::string(), false);
	}

	resp.setConnClose(!keepAlive);

	respondWithObjectRef(clientSocket, resp);
}


std::string Server::getFilenameFromTarget(std::string target) {
	if (target[0] == '/') {
		return (serverRoot / target.substr(1)).string();
	}
	else {
		return (serverRoot / target).string();
	}
}


std::string replaceParams(std::string target, std::map<std::string, std::string> params) {
	auto it = target.begin();
	auto oldIt = it;
	std::string result;

	while ((it = find(it, target.end(), '{')) != target.end()) {
		result += std::string(oldIt, it);
		auto endIt = find(it, target.end(), '}');
		if (endIt == target.end()) {
			BOOST_THROW_EXCEPTION(routeError() << stringInfoFromFormat("Error: unmatched paranthesis in route target %1%", target));
		}

		std::string paramName(it + 1, endIt);
		auto paramFound = params.find(paramName);
		if (paramFound == params.end()) {
			BOOST_THROW_EXCEPTION(routeError() << stringInfoFromFormat("Error: parameter name %1% in route target %2% not found",
				paramName, target));
		}
		result += paramFound->second;
		++it;
		oldIt = it;
	}
	result += std::string(oldIt, it);

	return result;
}


Response Server::getResponseFromSource(std::string filename, Request& request) {
	filename = expandFilename(filename);

	if (!bf::exists(filename)) {
		BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Error: File not found: %1%", filename));
	}

	Response result(500, "", true);

	bool isDynamic = getPymlIsDynamic(filename);
	CacheController::CachePragma cachePragma = cacheController.getCacheControl(
		relative(filename, serverRoot).string(), !isDynamic);

	std::string etag;
	if (request.headerExists("if-none-match")) {
		etag = request.getHeader("if-none-match").get();
		etag = etag.substr(1, etag.length() - 2);
	}
	if (cachePragma.isStore && serverCache.checkCacheTag(filename, etag)) {
		result = Response(304, "", false);
	}
	else {
		IteratorResult pymlResult = getPymlResultRequestCache(filename);

		std::multimap<std::string, std::string> headersMap = PythonModule::krait.getGlobalTupleList("extra_headers");
		std::unordered_multimap<std::string, std::string> headers(headersMap.begin(), headersMap.end());


		if (!PythonModule::krait.checkIsNone("response")) {
			result = Response(PythonModule::krait.eval("str(response)"));
			for (const auto& header : headers) {
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


void Server::startWebsocketsServer(int clientSocket, Request& request) {
	Response resp(500, "", true);

	try {
		request.setRouteVerb(RouteVerb::WEBSOCKET);

		std::map<std::string, std::string> params;
		const Route& route = Route::getRouteMatch(config.getRoutes(), request.getRouteVerb(), request.getUrl(), params);

		std::string targetReplaced = replaceParams(route.getTarget(request.getUrl()), params);
		std::string sourceFile = getFilenameFromTarget(targetReplaced);

		PythonModule::krait.setGlobalRequest("request", request);
		PythonModule::krait.setGlobal("url_params", params);
		PythonModule::krait.setGlobal("extra_headers", std::multimap<std::string, std::string>());
		PythonModule::websockets.run("request = WebsocketsRequest(krait.request)");

		resp = getResponseFromSource(sourceFile, request);
	}
	catch (notFoundError& ex) {
		DBG_FMT("notFound: %1%", ex.what());
		resp = Response(404, "<html><body><h1>404 Not Found</h1></body></html>", true);
	}
	catch (rootException& ex) {
		resp = Response(500, "<html><body><h1>500 Internal Server Error</h1></body></html>", true);
		Loggers::logErr(formatString("Error serving client: %1%", ex.what()));
	}

	if (resp.getStatusCode() >= 200 && resp.getStatusCode() < 300) {
		WebsocketsServer server(clientSocket);
		server.start(request);
	}
	else {
		resp.setConnClose(!keepAlive);
		respondWithObjectRef(clientSocket, resp);
	}
}


bool Server::canContainPython(std::string filename) {
	return ba::ends_with(filename, ".html") || ba::ends_with(filename, ".htm") || ba::ends_with(filename, ".pyml");
}


std::string Server::expandFilename(std::string filename) {
	if (bf::is_directory(filename)) {
		//DBG("Converting to directory automatically");
		filename = (bf::path(filename) / "index").string(); //Not index.html; taking care below about it
	}
	if (pathBlocked(filename)) {
		DBG_FMT("Blocking bf::path %1%", filename);
		BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Error: File not found: %1%", filename));
	}

	if (!bf::exists(filename)) {
		if (bf::exists(filename + ".html")) {
			//DBG("Adding .html automatically");
			filename += ".html";
		}
		else if (bf::exists(filename + ".htm")) {
			//DBG("Adding .htm automatically");
			filename += ".htm";
		}
		else if (bf::exists(filename + ".pyml")) {
			//DBG("Adding .pyml automatically");
			filename += ".pyml";
		}
		else if (bf::exists(filename + ".py")) {
			filename += ".py";
		}

		else if (bf::path(filename).extension() == ".html" &&
			bf::exists(bf::change_extension(filename, ".htm"))) {
			filename = bf::change_extension(filename, "htm").string();
			//DBG("Changing extension to .htm");
		}
		else if (bf::path(filename).extension() == ".htm" &&
			bf::exists(bf::change_extension(filename, ".html"))) {
			filename = bf::change_extension(filename, "html").string();
			//DBG("Changing extension to .html");
		}
	}
	return filename;
}


IteratorResult Server::getPymlResultRequestCache(std::string filename) {
	//DBG("Reading pyml cache");
	interpretCacheRequest = true;
	const IPymlFile* pymlFile = serverCache.get(filename);
	return IteratorResult(PymlIterator(pymlFile->getRootItem()));
}

bool Server::getPymlIsDynamic(std::string filename) {
	interpretCacheRequest = true;
	const IPymlFile* pymlFile = serverCache.get(filename);
	return pymlFile->isDynamic();
}

PymlFile* Server::constructPymlFromFilename(std::string filename, boost::object_pool<PymlFile>& pool, char* tagDest) {
	DBG_FMT("constructFromFilename(%1%)", filename);
	std::string source = readFromFile(filename);
	generateTagFromStat(filename, tagDest);
	std::unique_ptr<IPymlParser> parser;
	if (canContainPython(filename)) {
		DBG("choosing v2 pyml parser");
		parser = std::unique_ptr<IPymlParser>(new V2PymlParser(serverCache));
	}
	else if (ba::ends_with(filename, ".py")) {
		parser = std::unique_ptr<IPymlParser>(new RawPythonPymlParser(serverCache));
	}
	else {
		parser = std::unique_ptr<IPymlParser>(new RawPymlParser());
	}
	return pool.construct(source.begin(), source.end(), parser);
}


void Server::onServerCacheMiss(std::string filename) {
	if (interpretCacheRequest) {
		cacheRequestPipe.pipeWrite(filename);
		Loggers::logInfo(formatString("Cache miss on url %1%", filename));
	}
}

void Server::updateParentCaches() {
	while (cacheRequestPipe.pipeAvailable()) {
		std::string filename = cacheRequestPipe.pipeRead();
		DBG("next cache add is in parent");

		interpretCacheRequest = false;
		serverCache.get(filename);
		interpretCacheRequest = true;
	}
}

bool Server::pathBlocked(std::string filename) {
	bf::path filePath(filename);
	for (auto& part : filePath) {
		if (part.c_str()[0] == '.') {
			DBG_FMT("BANNED because of part %1%", part.c_str());
			return true;
		}
	}
	return false;
}


void Server::addDefaultHeaders(Response& response, std::string filename, Request& request) {
	if (!response.headerExists("Content-Type")) {
		response.setHeader("Content-Type", getContentType(filename));
	}
	std::time_t timeVal = std::time(NULL);
	if (timeVal != -1 && !response.headerExists("Date")) {
		response.setHeader("Date", unixTimeToString(timeVal));
	}
}


std::string Server::getContentType(std::string filename) {
	std::string extension;

	if (PythonModule::krait.checkIsNone("_content_type")) {
		DBG("No content_type set");
		bf::path filePath(filename);
		extension = filePath.extension().string();
		if (extension == ".pyml") {
			extension = filePath.stem().extension().string();
		}
	}
	else {
		std::string varContentType = PythonModule::krait.getGlobalStr("_content_type");
		DBG_FMT("content_type set to %1%", varContentType);

		if (!ba::starts_with(varContentType, "ext/")) {
			return varContentType;
		}
		else {
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

void Server::addStandardCacheHeaders(Response& response, std::string filename, CacheController::CachePragma pragma) {
	response.setHeader("cache-control", cacheController.getValueFromPragma(pragma));

	if (pragma.isStore) {
		response.addHeader("etag", "\"" + serverCache.getCacheTag(filename) + "\"");
	}
}

void Server::loadContentTypeList() {
	bf::path contentFilePath = getExecRoot() / "globals" / "mime.types";
	std::ifstream mimeFile(contentFilePath.string());

	char line[1024];
	std::string mimeType;

	while (mimeFile.getline(line, 1023)) {
		if (line[0] == '\0' || line[0] == '#') {
			continue;
		}

		const char* separators = " \t\r\n";
		char* ptr = strtok(line, separators);
		if (ptr == NULL) {
			continue;
		}

		mimeType.assign(ptr);
		ptr = strtok(NULL, separators);
		while (ptr != NULL) {
			//This SHOULD be fine.
			//Make it an extension (html to .html)
			*(ptr - 1) = '.';
			contentTypeByExtension[std::string(ptr - 1)] = mimeType;

			ptr = strtok(NULL, separators);
		}

	}
}
