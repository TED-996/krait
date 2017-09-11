#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include "networkManager.h"
#include "except.h"
#include "managedSocket.h"

#define DBG_DISABLE
#include"dbg.h"


NetworkManager::NetworkManager(int socket){
	this->socket = socket;
	this->listening = false;
}


NetworkManager::NetworkManager(NetworkManager&& source) noexcept {
	this->socket = source.socket;
	this->listening = source.listening;

	source.socket = InvalidSocket;
	source.listening = false;
}

NetworkManager::~NetworkManager() {
	if (this->socket != InvalidSocket) {
		::close(socket);
		this->socket = InvalidSocket;
	}
}


NetworkManager NetworkManager::fromAnyOnPort(short port) {
	struct sockaddr_in serverSockaddr;
	memset(&serverSockaddr, 0, sizeof(serverSockaddr));

	serverSockaddr.sin_family = AF_INET;
	serverSockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverSockaddr.sin_port = htons(port);

	int sd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (sd == -1) {
		BOOST_THROW_EXCEPTION(networkError()
			<< stringInfo("getListenSocket: could not create socket in NetworkManager::fromAnyOnPort.") << errcodeInfoDef());
	}

	const int reuseAddr = 1;
	int enable = (int)reuseAddr;
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
		BOOST_THROW_EXCEPTION(networkError()
			<< stringInfo("getListenSocket: coult not set reuseAddr in NetworkManager::fromAnyOnPort.") << errcodeInfoDef());
	}

	if (bind(sd, (sockaddr*)&serverSockaddr, sizeof(sockaddr)) != 0) {
		BOOST_THROW_EXCEPTION(networkError()
			<< stringInfo("getListenSocket: could not bind socket in NetworkManager::fromAnyOnPort.") << errcodeInfoDef());
	}

	return NetworkManager(sd);
}

int NetworkManager::getFd() {
	return this->socket;
}

void NetworkManager::initialize() {
}

bool NetworkManager::listen(size_t backlog) {
	if (backlog == -1) {
		backlog = SOMAXCONN;
	}

	if (this->socket == InvalidSocket) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("NetworkManager used (listen) after deconstruction."));
	}
	if (listening) {
		return true; //Make it idempotent.
	}

	this->listening = (::listen(this->socket, (int)backlog) == 0);
	if (!this->listening) {
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("listen(): Setting socket to listening.") << errcodeInfoDef());
	}
	return this->listening;
}

std::unique_ptr<IManagedSocket> NetworkManager::accept() {
	if (socket == InvalidSocket) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("NetworkManager used (accept) after deconstruction."));
	}
	if (!listening) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("NetworkManager not listening before call to accept()"));
	}

	int newSocket = ::accept(this->socket, nullptr, nullptr);
	if (newSocket == -1) {
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("accept(): accepting new socket in NetworkManager"));
	}
	return std::make_unique<ManagedSocket>(newSocket);
}

std::unique_ptr<IManagedSocket> NetworkManager::acceptTimeout(int timeoutMs) {
	if (socket == InvalidSocket) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("NetworkManager used (acceptTimeout) after deconstruction."));
	}
	if (!listening) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("NetworkManager not listening before call to acceptTimeout()"));
	}

	pollfd pfd;
	pfd.fd = this->socket;
	pfd.events = POLLIN;
	pfd.revents = 0;

	int pollResult = poll(&pfd, 1, timeoutMs);
	if (pollResult < 0) {
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("poll(): waiting to accept new client."));
	}
	if (pollResult == 0) {
		return nullptr;
	}
	
	return accept();
}
