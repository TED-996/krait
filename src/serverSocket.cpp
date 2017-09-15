#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include "serverSocket.h"
#include "except.h"
#include "managedSocket.h"

#define DBG_DISABLE
#include"dbg.h"


ServerSocket::ServerSocket(int socket){
	this->socket = socket;
	this->listening = false;
}


ServerSocket::ServerSocket(ServerSocket&& other) noexcept {
	this->socket = other.socket;
	this->listening = other.listening;

	other.socket = InvalidSocket;
	other.listening = false;
}

ServerSocket::~ServerSocket() {
	if (this->socket != InvalidSocket) {
		::close(socket);
		this->socket = InvalidSocket;
	}
	this->listening = false;
}


ServerSocket& ServerSocket::operator=(ServerSocket&& other) noexcept {
	if (this->socket != InvalidSocket) {
		::close(socket);
	}

	this->socket = other.socket;
	this->listening = other.listening;

	other.socket = InvalidSocket;
	other.listening = false;

	return *this;
}

ServerSocket ServerSocket::fromAnyOnPort(u_int16_t port) {
	struct sockaddr_in serverSockaddr;
	memset(&serverSockaddr, 0, sizeof(serverSockaddr));

	serverSockaddr.sin_family = AF_INET;
	serverSockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverSockaddr.sin_port = htons(port);

	int sd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (sd == -1) {
		BOOST_THROW_EXCEPTION(networkError()
			<< stringInfo("socket: could not create socket in ServerSocket::fromAnyOnPort.") << errcodeInfoDef());
	}

	const int reuseAddr = 1;
	int enable = (int)reuseAddr;
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
		BOOST_THROW_EXCEPTION(networkError()
			<< stringInfo("setsockopt: coult not set reuseAddr in ServerSocket::fromAnyOnPort.") << errcodeInfoDef());
	}

	if (bind(sd, (sockaddr*)&serverSockaddr, sizeof(sockaddr)) != 0) {
		BOOST_THROW_EXCEPTION(networkError()
			<< stringInfo("bind: could not bind socket in ServerSocket::fromAnyOnPort.") << errcodeInfoDef());
	}

	return ServerSocket(sd);
}

int ServerSocket::getFd() {
	return this->socket;
}

void ServerSocket::initialize() {
}

bool ServerSocket::listen(size_t backlog) {
	if (backlog == -1) {
		backlog = SOMAXCONN;
	}

	if (this->socket == InvalidSocket) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("ServerSocket used (listen) after deconstruction."));
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

std::unique_ptr<IManagedSocket> ServerSocket::accept() {
	if (socket == InvalidSocket) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("ServerSocket used (accept) after deconstruction."));
	}
	if (!listening) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("ServerSocket not listening before call to accept()"));
	}

	int newSocket = ::accept(this->socket, nullptr, nullptr);
	if (newSocket == -1) {
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("accept(): accepting new socket in ServerSocket") << errcodeInfoDef());
	}
	return std::make_unique<ManagedSocket>(newSocket);
}

std::unique_ptr<IManagedSocket> ServerSocket::acceptTimeout(int timeoutMs) {
	if (socket == InvalidSocket) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("ServerSocket used (acceptTimeout) after deconstruction."));
	}
	if (!listening) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("ServerSocket not listening before call to acceptTimeout()"));
	}

	pollfd pfd;
	pfd.fd = this->socket;
	pfd.events = POLLIN;
	pfd.revents = 0;

	int pollResult = poll(&pfd, 1, timeoutMs);
	if (pollResult < 0) {
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("poll(): waiting to accept new client.") << errcodeInfoDef());
	}
	if (pollResult == 0) {
		return nullptr;
	}
	
	return accept();
}
