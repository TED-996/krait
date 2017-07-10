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

//#define DBG_DISABLE
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
		          std::placeholders::_2),
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
		
		PythonModule::krait.setGlobalRequest("request", request);
		PythonModule::krait.setGlobal("url_params", params);
		PythonModule::krait.setGlobal("extra_headers", std::multimap<std::string, std::string>());
		
		bool skipDenyTest;
		std::string sourceFile;
		if (!route.isMvcRoute()) {
			std::string targetReplaced = replaceParams(route.getTarget(request.getUrl()), params);
			sourceFile = getFilenameFromTarget(targetReplaced);
			skipDenyTest = false;
		}
		else {
			//TODO: refactor, too many responsibilities...
			//Maybe get a PageRenderer going...
			Loggers::logInfo("executing ctrl route");
			PythonModule::main.setGlobal("ctrl", PythonModule::main.callObject(route.getCtrlClass()));
			sourceFile = PythonModule::main.eval("krait.get_full_path(ctrl.get_view())");
			skipDenyTest = true;
		}

		Loggers::logInfo(formatString("getting resp from source %1%", sourceFile));
		resp = getResponseFromSource(sourceFile, request, skipDenyTest);
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

		resp = getResponseFromSource(sourceFile, request, false);
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


IteratorResult Server::getPymlResultRequestCache(std::string filename) {
	//DBG("Reading pyml cache");
	interpretCacheRequest = true;
	const IPymlFile& pymlFile = serverCache.get(filename);
	return IteratorResult(PymlIterator(pymlFile.getRootItem()));
}

bool Server::getPymlIsDynamic(std::string filename) {
	interpretCacheRequest = true;
	const IPymlFile& pymlFile = serverCache.get(filename);
	return pymlFile.isDynamic();
}

std::unique_ptr<PymlFile> Server::constructPymlFromFilename(std::string filename, PymlCache::CacheTag& tagDest) {
	DBG_FMT("constructFromFilename(%1%)", filename);
	std::string source = readFromFile(filename);
	tagDest.setTag(generateTagFromStat(filename));

	std::unique_ptr<IPymlParser> parser;
	if (canContainPython(filename)) {
		DBG("choosing v2 pyml parser");
		parser = std::make_unique<V2PymlParser>(serverCache);
	}
	else if (ba::ends_with(filename, ".py")) {
		parser = std::make_unique<RawPythonPymlParser>(serverCache);
	}
	else {
		parser = std::make_unique<RawPymlParser>();
	}
	//return pool.construct(source.begin(), source.end(), parser);
	return std::make_unique<PymlFile>(source.begin(), source.end(), parser);
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

