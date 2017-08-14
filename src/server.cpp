#include <unistd.h>
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
#include "logger.h"
#include "rawPymlParser.h"
#include "v2PymlParser.h"
#include "websocketsServer.h"
#include "rawPythonPymlParser.h"
#include "signalManager.h"
#include "config.h"

#define DBG_DISABLE
#include"dbg.h"
#include "networkManager.h"


namespace b = boost;
namespace bf = boost::filesystem;
namespace ba = boost::algorithm;

Server* Server::instance = nullptr;

Server::Server(std::string serverRoot, int port)
	:
	serverRoot(bf::path(serverRoot)),
	config(),
	cacheController(config, serverRoot),
	networkManager(std::make_unique<NetworkManager>(std::move(NetworkManager::fromAnyOnPort(port)))),
	clientSocket(nullptr),
	serverCache(
		std::bind(&Server::constructPymlFromFilename,
		          this,
		          std::placeholders::_1,
		          std::placeholders::_2),
		std::bind(&Server::onServerCacheMiss, this, std::placeholders::_1)),
	responseBuilder(this->serverRoot, config, cacheController, serverCache){

	if (Server::instance != nullptr) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Multiple Server instances!"));
	}
	Server::instance = this;

	this->serverRoot = bf::path(serverRoot);
	
	DBG("routes got");

	
	try {
		PythonModule::initModules(serverRoot);
	}
	catch (pythonError& err) {
		Loggers::logErr(formatString("Error running init.py: %1%", err.what()));
		exit(1);
	}

	DBG("python initialized");

	config.load();
	cacheController.load();

	Loggers::logInfo(formatString("Server initialized on port %1%", port));

	stdinDisconnected = fdClosed(0);
	
	shutdownRequested = false;
}


Server::~Server() {
}

void Server::runServer() {
	try {
		networkManager->listen(5); //TODO: backlog size?
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
	std::unique_ptr<IManagedSocket> newClient;
	try {
		newClient = std::move(networkManager->acceptTimeout(timeout));
	}
	catch (networkError err) {
		Loggers::errLogger.log("Could not get new client.");
		exit(1);
	}

	if (newClient == nullptr) {
		return;
	}

	pid_t pid = fork();
	if (pid == -1) {
		BOOST_THROW_EXCEPTION(syscallError()
			<< stringInfo("fork(): creating process to serve socket. Is the system out of resources?") << errcodeInfoDef());
	}
	if (pid == 0) {
		SignalManager::clearPids();
		clientSocket = std::move(newClient);
		networkManager.reset(); // Destruct the network manager to close the socket.

		cacheRequestPipe.closeRead();

		serveClientStart();
		exit(255); //The function above should call exit()!
	}
	newClient.reset(); // Destruct the new client to close the socket.

	SignalManager::addPid((int)pid);
}


void Server::tryCheckStdinClosed() const {
	if (!stdinDisconnected && fdClosed(0)) {
		raise(SIGUSR1); //TODO: change to SIGUSR2 when proper shutdown is implemented.
	}
}

void Server::serveClientStart() {
	Loggers::logInfo("Serving a new client");
	bool isHead = false;
	keepAliveTimeoutSec = maxKeepAliveSec;

	try {
		while (true) {
			b::optional<Request> requestOpt = clientSocket->getRequestTimeout(keepAliveTimeoutSec * 1000);
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
						serveRequestWebsockets(request);
					}
					else {
						serveRequest(request);
					}

					Loggers::logInfo("Serving a request finished.");
				}
				catch (networkError&) {
					Loggers::errLogger.log("Could not respond to client request (network error - disconnected?).");
					clientSocket.reset();
					exit(1);
				}
				catch (pythonError& err) {
					Loggers::errLogger.log(formatString("Python error:\n%1%", err.what()));
					clientSocket.reset();
					exit(1);
				}
				
				clientSocket.reset();
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
			clientSocket->respondWithObject(std::move(Response(500, "", true)));
		}
		else {
			clientSocket->respondWithObject(std::move(Response(500, "<html><body><h1>500 Internal Server Error</h1></body></html>", true)));
		}
		Loggers::logErr(formatString("Error serving client: %1%", ex.what()));
	}


	clientSocket.reset();
	exit(0);
}

void Server::serveRequest(Request& request) {
	Response resp(500, "", true);

	bool isHead = false;

	try {
		interpretCacheRequest = true;
		resp = responseBuilder.buildResponse(request);
	}
	catch (rootException& ex) {
		resp = Response(500, "<html><body><h1>500 Internal Server Error</h1></body></html>", true);
		Loggers::logErr(formatString("Error serving client: %1%", ex.what()));
	}

	if (isHead) {
		resp.setBody(std::string(), false);
	}

	resp.setConnClose(!keepAlive);

	clientSocket->respondWithObject(std::move(resp));
}

void Server::serveRequestWebsockets(Request& request) {
	Response resp(500, "", true);
	try {
		interpretCacheRequest = true;
		resp = responseBuilder.buildWebsocketsResponse(request);
	}
	catch (rootException& ex) {
		resp = Response(500, "<html><body><h1>500 Internal Server Error</h1></body></html>", true);
		Loggers::logErr(formatString("Error serving client: %1%", ex.what()));
	}

	if (resp.getStatusCode() >= 200 && resp.getStatusCode() < 300) {
		WebsocketsServer server(*clientSocket);
		server.start(request);
	}
	else {
		resp.setConnClose(!keepAlive);
		clientSocket->respondWithObject(std::move(resp));
	}
}

bool Server::canContainPython(std::string filename) {
	return ba::ends_with(filename, ".html") || ba::ends_with(filename, ".htm") || ba::ends_with(filename, ".pyml");
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

