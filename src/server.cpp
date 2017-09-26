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

//#define DBG_DISABLE
#include"dbg.h"
#include "dbgStopwatch.h"


namespace b = boost;
namespace bf = boost::filesystem;
namespace ba = boost::algorithm;


Server* Server::instance = nullptr;


Server::Server(ArgvConfig argvConfig)
	:
	serverRoot(bf::canonical(bf::absolute(argvConfig.getSiteRoot()))),
	pythonInitializer(argvConfig.getSiteRoot()),
	config(argvConfig),
	cacheController(config, argvConfig.getSiteRoot()),
	networkManager(config.getHttpPort(), config.getHttpsPort(), config),
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

	if (argvConfig.getHttpPort() != boost::none) {
		Loggers::logInfo(formatString("HTTP Server initialized on port %1%", argvConfig.getHttpPort().get()));
	}
	if (argvConfig.getHttpsPort() != boost::none) {
		Loggers::logInfo(formatString("HTTPS Server initialized on port %1%", argvConfig.getHttpsPort().get()));
	}


	stdinDisconnected = fdClosed(0);
	
	shutdownRequested = false;
}

Server::~Server() {
}

void Server::runServer() {
	try {
		networkManager.listen();
	}
	catch (const networkError&) {
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
	Loggers::logInfo("Server shutting down.");
}

void Server::requestShutdown() {
	shutdownRequested = true;
	Loggers::logInfo("Shutdown requested, waiting...");
}

void Server::cleanup() {
}


void Server::tryAcceptConnection() {
	const int timeout = 100;
	std::unique_ptr<IManagedSocket> newClient;
	try {
		newClient = networkManager.acceptTimeout(timeout);
	}
	catch (const networkError&) {
		Loggers::errLogger.log("Could not get new client: network error.");
		exit(1);
	}
	catch (const sslError&) {
		Loggers::errLogger.log("Could not get new client: SSL error.");
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
		try {
			SignalManager::clearPids();
			clientSocket = std::move(newClient);
			clientSocket->atFork();

			//networkManager.detachContext();
			networkManager.close(); // Close the listen sockets.

			cacheRequestPipe.closeRead();

			serveClientStart();
		}
		catch(const std::exception& ex) {
			Loggers::logErr(formatString("Exception serving client: %1%", ex.what()));
		}
		exit(255); //The function above should call exit()!
	}
	newClient->detachContext();
	newClient.reset(); // Destruct the new client to close the socket.

	SignalManager::addPid((int)pid);
}


void Server::tryCheckStdinClosed() const {
	if (!stdinDisconnected && fdClosed(0)) {
		raise(SIGUSR2);
	}
}

void Server::serveClientStart() {
	Loggers::logInfo("Serving a new client");
	bool isHead = false;
	keepAliveTimeoutSec = maxKeepAliveSec;
	clientSocket->initialize();

	try {
		while (!shutdownRequested) {
			std::unique_ptr<Request> requestPtr = clientSocket->getRequestTimeout(keepAliveTimeoutSec * 1000);
			if (requestPtr == nullptr) {
				Loggers::logInfo(formatString("Requests finished."));
				break;
			}

			DbgStopwatch stopwatch(formatString("Serving request %1% %2%", httpVerbToString(requestPtr->getVerb()), requestPtr->getUrl()));
			(void)stopwatch;
			
			Request& request = *requestPtr;
			Loggers::logInfo(formatString("REQUEST: %1% %2%", httpVerbToString(request.getVerb()), request.getUrl()));

			if (request.getVerb() == HttpVerb::HEAD) {
				isHead = true;
			}

			/*
			//If-Modified-Since unsuported.
			if (request.headerExists("If-Modified-Since")) {
				Loggers::logInfo(formatString("Client tried If-Modified-Since with date %1%. Unsupported.",
					*request.getHeader("If-Modified-Since")));
			}
			*/

			keepAliveTimeoutSec = std::min(maxKeepAliveSec, request.getKeepAliveTimeout());
			keepAlive = request.isKeepAlive() && keepAliveTimeoutSec != 0 && !request.isUpgrade("websocket") && !shutdownRequested;

			if (request.isUpgrade("websocket")) {
				serveRequestWebsockets(request);
			}
			else {
				serveRequest(request);
			}

			Loggers::logInfo("Serving a request finished.");

			if (!keepAlive) {
				break;
			}
		}
	}
	catch (networkError& ex) {
		Loggers::logErr("Client disconnected.");
		DBG_FMT("Network erorr:\n%1%", ex.what());
	}
	catch (sslError& ex) {
		Loggers::logErr("Client SSL error.");
		DBG_FMT("SSL error:\n%1%", ex.what());
	}
	catch (rootException& ex) {
		if (isHead) {
			clientSocket->respondWithObject(Response(500, "", true));
		}
		else {
			clientSocket->respondWithObject(Response(500, "<html><body><h1>500 Internal Server Error</h1></body></html>", true));
		}
		Loggers::logErr(formatString("Error serving client:\n%1%", ex.what()));
	}
	catch (std::exception& ex) {
		Loggers::logErr(formatString("Unexpected error!\n%1%", ex.what()));
	}

	clientSocket.reset();
	exit(0);
}

void Server::serveRequest(Request& request) {
	std::unique_ptr<Response> resp;

	bool isHead = false;

	try {
		interpretCacheRequest = true;
		resp = responseBuilder.buildResponse(request);
	}
	catch (rootException& ex) {
		resp = std::make_unique<Response>(500, "<html><body><h1>500 Internal Server Error</h1></body></html>", true);
		Loggers::logErr(formatString("Error serving client: %1%", ex.what()));
	}

	if (isHead) {
		resp->setBody(std::string(), false);
	}

	resp->setConnClose(!keepAlive);

	clientSocket->respondWithObject(std::move(*resp));
}

void Server::serveRequestWebsockets(Request& request) {
	std::unique_ptr<Response> resp;

	try {
		interpretCacheRequest = true;
		resp = responseBuilder.buildWebsocketsResponse(request);
	}
	catch (rootException& ex) {
		resp = std::make_unique<Response>(500, "<html><body><h1>500 Internal Server Error</h1></body></html>", true);
		Loggers::logErr(formatString("Error serving client: %1%", ex.what()));
	}

	if (resp->getStatusCode() >= 200 && resp->getStatusCode() < 300) {
		WebsocketsServer server(*clientSocket);
		server.start(request);
	}
	else {
		resp->setConnClose(!keepAlive);
		clientSocket->respondWithObject(std::move(*resp));
	}
}

bool Server::canContainPython(const std::string& filename) {
	return ba::ends_with(filename, ".html") || ba::ends_with(filename, ".htm") || ba::ends_with(filename, ".pyml");
}

std::unique_ptr<PymlFile> Server::constructPymlFromFilename(const std::string& filename, PymlCache::CacheTag& tagDest) {
	std::string source = readFromFile(filename);
	tagDest.setTag(generateTagFromStat(filename));

	std::unique_ptr<IPymlParser> parser;
	if (canContainPython(filename)) {
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


void Server::onServerCacheMiss(const std::string& filename) {
	if (interpretCacheRequest) {
		Loggers::logInfo(formatString("Cache miss on url %1%", filename));
		cacheRequestPipe.pipeWrite(filename);
	}
}

void Server::updateParentCaches() {
	interpretCacheRequest = false;

	while (cacheRequestPipe.pipeAvailable()) {
		std::string filename = cacheRequestPipe.pipeRead();
		DBG("Server: next cache add is in parent");

		serverCache.get(filename);
	}

	interpretCacheRequest = true;
}

