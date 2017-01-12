#include <algorithm>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include "network.h"
#include "utils.h"
#include "except.h"

#define DBG_DISABLE
#include "dbg.h"

using namespace std;


int getServerSocket(int port, bool setListen) {
	sockaddr_in serverSockaddr;
	memzero(serverSockaddr);

	serverSockaddr.sin_family = AF_INET;
	serverSockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverSockaddr.sin_port = htons(port);

	int sd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (sd == -1) {
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("getListenSocket: could not create socket!"));
	}

	if (bind(sd, (sockaddr*)&serverSockaddr, sizeof(sockaddr)) != 0) {
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("getListenSocket: could not bind socket") << errcodeInfo(errno));
	}

	if (setListen) {
		if (listen(sd, 1024) == -1) {
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("getListenSocket: could not set socket to listen") << errcodeInfo(
			                          errno));
		}
	}

	return sd;
}


void setSocketListen(int sd) {
	if (listen(sd, 1024) == -1) {
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("getListenSocket: could not set socket to listen") << errcodeInfo(
		                          errno));
	}
}


int getNewClient(int listenerSocket, int timeout) {
	sockaddr_in clientSockaddr;
	memzero(clientSockaddr);

	pollfd pfd;
	pfd.fd = listenerSocket;
	pfd.events = POLLIN;
	pfd.revents = 0;

	int pollResult = poll(&pfd, 1, timeout);
	if (pollResult == -1) {
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("getNewClient: poll() failed") << errcodeInfo(errno));
	}
	if (pollResult == 0) {
		return -1;
	}

	socklen_t sockaddrLen = sizeof(clientSockaddr);
	int client = accept(listenerSocket, (sockaddr*)&clientSockaddr, &sockaddrLen);

	if (client < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return -1;
		}

		BOOST_THROW_EXCEPTION(networkError() << stringInfo("getNewClient: could not accept new client") << errcodeInfo(errno));
	}

	return client;
}

void closeSocket(int clientSocket) {
	close(clientSocket);
}


void printSocket(int clientSocket) {
	char data[4096];
	memzero(data);
	int readBytes;
	while ((readBytes = read(clientSocket, data, 4095)) != 0) {
		if (readBytes < 0) {
			BOOST_THROW_EXCEPTION(syscallError() << stringInfo("printSocket: error at read().") << errcodeInfo(errno));
		}

		DBG_FMT("read %1% bytes", readBytes);
		printf("%s\n", data);
	}
}

Request getRequestFromSocket(int clientSocket) {
	RequestParser parser;
	char buffer[4096];
	int bytesRead;

	while (!parser.isFinished() && (bytesRead = read(clientSocket, buffer, sizeof(buffer))) > 0) {
		if (bytesRead < 0) {
			BOOST_THROW_EXCEPTION(syscallError() << stringInfo("getRequestFromSocket: error at read().") << errcodeInfo(errno));
		}

		parser.consume(buffer, bytesRead);
	}

	return parser.getRequest();
}


void respondWithCString(int clientSocket, const char* response);
void respondWithBuffer(int clientSocket, const char* response, size_t size);

void respondRequestHttp10(int clientSocket) {
	respondWithCString(clientSocket, "HTTP/1.0 505 Version Not Supported\r\nConnection:Close\r\n\r\n");
}

void respondRequest404(int clientSocket) {
	respondWithCString(clientSocket, "HTTP/1.0 404 Not Found\r\nConnection:Close\r\n\r\n");
}


void respondRequest200(int clientSocket) {
	respondWithCString(clientSocket, "HTTP/1.0 200 OK\r\nConnection:Close\r\n\r\n");
}


void respondWithObject(int clientSocket, Response response) {
	string responseData = response.getResponseData();

	respondWithBuffer(clientSocket, responseData.c_str(), responseData.length());
}


void respondWithCString(int clientSocket, const char* response) {
	respondWithBuffer(clientSocket, response, strlen(response));
}


void respondWithBuffer(int clientSocket, const char* response, size_t size) {
	size_t lenLeft = size;
	const size_t maxBlockSize = 65536;

	while (lenLeft > 0) {
		size_t lenCurrent = min(maxBlockSize, lenLeft);
		if (write(clientSocket, response, lenCurrent) != (int)lenCurrent) {
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("respondWith: could not send response.") << errcodeInfo(errno));
		}
		response += lenCurrent;
		lenLeft -= lenCurrent;
	}
}




