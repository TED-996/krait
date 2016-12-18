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
#include "network.h"
#include "utils.h"
#include "except.h"

#include "dbg.h"

using namespace std;


int getListenSocket(int port) {
	sockaddr_in serverSockaddr;
	memzero(serverSockaddr);

	serverSockaddr.sin_family = AF_INET;
	serverSockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverSockaddr.sin_port = htons(port);

	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("getListenSocket: could not create socket!"));
	}

	if (bind(sd, (sockaddr*)&serverSockaddr, sizeof(sockaddr)) != 0) {
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("getListenSocket: could not bind socket") << errcodeInfo(errno));
	}

	if (listen(sd, 1024) == -1) {
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("getListenSocket: could not set socket to listen") << errcodeInfo(
		                          errno));
	}

	return sd;
}


int getNewClient(int listenerSocket) {
	sockaddr_in clientSockaddr;
	memzero(clientSockaddr);

	socklen_t sockaddrLen = sizeof(clientSockaddr);
	int client = accept(listenerSocket, (sockaddr*)&clientSockaddr, &sockaddrLen);

	if (client < 0) {
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("getNewClient: could not accept new client"));
	}

	return client;
}

void closeSocket(int clientSocket) {
	close(clientSocket);
}


void printSocket(int clientSocket) {
	char data[4096];
	memzero(data);
	size_t readBytes;
	while ((readBytes = read(clientSocket, data, 4095)) != 0) {
		DBG_FMT("read %1% bytes", readBytes);
		printf("%s\n", data);
	}
}

Request getRequestFromSocket(int clientSocket) {
	RequestParser parser;
	char buffer[4096];
	int bytesRead;

	while (!parser.isFinished() && (bytesRead = read(clientSocket, buffer, sizeof(buffer))) > 0) {
		parser.consume(buffer, bytesRead);
	}

	return parser.getRequest();
}


void respondWithBuffer(int clientSocket, const char* response);

void respondRequestHttp10(int clientSocket) {
	respondWithBuffer(clientSocket, "HTTP/1.0 505 Version Not Supported\r\nConnection:Close\r\n\r\n");
}

void respondRequest404(int clientSocket) {
	respondWithBuffer(clientSocket, "HTTP/1.0 404 Not Found\r\nConnection:Close\r\n\r\n");
}


void respondRequest200(int clientSocket) {
	respondWithBuffer(clientSocket, "HTTP/1.0 200 OK\r\nConnection:Close\r\n\r\n");
}


void respondWithObject(int clientSocket, Response response) {
	const char* responseData = response.getResponseData().c_str();

	respondWithBuffer(clientSocket, responseData);
}


void respondWithBuffer(int clientSocket, const char* response) {
	size_t lenLeft = strlen(response);
	const size_t maxBlockSize = 65536;

	while (lenLeft > 0) {
		size_t lenCurrent = min(maxBlockSize, lenLeft);
		if (write(clientSocket, response, lenCurrent) != (int)lenCurrent) {
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("respondWith: could not send response."));
		}
		lenLeft -= lenCurrent;
	}
}
