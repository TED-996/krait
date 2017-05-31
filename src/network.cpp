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
#include <endian.h>
#include <cstdint>
#include "logger.h"
#include "network.h"
#include "utils.h"
#include "except.h"
#include "requestParser.h"

#define DBG_DISABLE
#include "dbg.h"

using namespace std;


int getServerSocket(int port, bool setListen, bool reuseAddr) {
	sockaddr_in serverSockaddr;
	memzero(serverSockaddr);

	serverSockaddr.sin_family = AF_INET;
	serverSockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverSockaddr.sin_port = htons(port);

	int sd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (sd == -1) {
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("getListenSocket: could not create socket.") << errcodeInfoDef());
	}
	
	int enable = (int)reuseAddr;
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1){
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("getListenSocket: coult not set reuseAddr.") << errcodeInfoDef());
	}

	if (bind(sd, (sockaddr*)&serverSockaddr, sizeof(sockaddr)) != 0) {
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("getListenSocket: could not bind socket") << errcodeInfoDef());
	}

	if (setListen) {
		if (listen(sd, 1024) == -1) {
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("getListenSocket: could not set socket to listen") << errcodeInfoDef());
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
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("poll(): getting new client.") << errcodeInfoDef());
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

		BOOST_THROW_EXCEPTION(networkError() << stringInfo("getNewClient: could not accept new client") << errcodeInfoDef());
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
			BOOST_THROW_EXCEPTION(syscallError() << stringInfo("read(): printing socket") << errcodeInfoDef());
		}

		printf("%s\n", data);
	}
}

boost::optional<Request> getRequestFromSocket(int clientSocket, int timeoutMs) {
	RequestParser parser;
	char buffer[4096];
	int bytesRead;
	int pollResult;

	pollfd pfd;
	pfd.fd = clientSocket;
	pfd.events = POLLIN;
	pfd.revents = 0;

	while (!parser.isFinished() && (pollResult = poll(&pfd, 1, timeoutMs)) != 0) {
		if (pollResult < 0){
			BOOST_THROW_EXCEPTION(syscallError() << stringInfo("poll(): waiting for request from socket."));
		}
		bytesRead = read(clientSocket, buffer, sizeof(buffer));
		if (bytesRead == 0){
			return boost::none;
		}
		if (bytesRead < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK){
				bytesRead = 0;
			}
			else{
				BOOST_THROW_EXCEPTION(syscallError() << stringInfo("read(): getting request from socket") << errcodeInfoDef());
			}
		}

		if (bytesRead != 0){
			try{
				parser.consume(buffer, bytesRead);
			}
			catch(httpParseError err){
				Loggers::logErr(formatString("Error parsing http request: %s\n", err.what()));
				return boost::none;
			}
		}
	}

	if (!parser.isFinished()){
		return boost::none;
	}

	return parser.getRequest();
}


bool readExactly(int clientSocket, void* dst, size_t nrBytes){
	while(nrBytes > 0){
		int bytesRead = read(clientSocket, dst, nrBytes);
		//DBG_FMT("[readExactly] Read %1% bytes, errno is %2%", bytesRead, errno);

		if (bytesRead == 0){
			return false;
		}

		if (bytesRead < 0){
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				bytesRead = 0;
			}
			else{
				return false;
			}
		}
		dst = (void*)((char*)dst + bytesRead);
		nrBytes -= bytesRead;
	}
	return true;
}


WebsocketsFrame getWebsocketsFrame(int clientSocket) {
	WebsocketsFrame result;

	uint8_t firstByte = 0;
	uint8_t secondByte = 0;
	uint64_t length = 0;
	uint32_t mask[4] = {0, 0, 0, 0};

	if (!readExactly(clientSocket, &firstByte, 1) || !readExactly(clientSocket, &secondByte, 1)){
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not read WebSockets frame: one of first 2 bytes missing.")
		                                     << errcodeInfoDef());
	}
	result.isFin = (bool) (firstByte & 0x80);
	result.opcode = (WebsocketsOpcode) (firstByte & 0xF);
	bool isMasked = (bool) (secondByte & 0x80);

	length = (uint64_t) (secondByte & 0x7F);
	if (length == 126){
		uint16_t lenTmp = 0;
		if (!readExactly(clientSocket, &lenTmp, 2)){
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not read Websockets frame: 16-bit length missing.")
												 << errcodeInfoDef());
		}
		length = ntohs(lenTmp);
	}
	else if (length == 127) {
		if (!readExactly(clientSocket, &length, 8)) {
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not read Websockets frame: 64-bit length missing.")
			                                     << errcodeInfoDef());
		}
		length = be64toh(length);
	}

	if (isMasked){
		if (!readExactly(clientSocket, &mask[0], 1) ||
			!readExactly(clientSocket, &mask[1], 1) ||
			!readExactly(clientSocket, &mask[2], 1) ||
			!readExactly(clientSocket, &mask[3], 1)){
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not read Websockets frame: mask missing.")
			                                     << errcodeInfoDef());
		}
	}
	unsigned char block[4096];

	while(length > 0){
		size_t sizeToRead = sizeof(block);
		if (length < sizeToRead){
			sizeToRead = (size_t)length;
		}
		int bytesRead = read(clientSocket, &block, sizeToRead);
		if (bytesRead < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK){
				bytesRead = 0;
			}
			else{
				BOOST_THROW_EXCEPTION(syscallError() << stringInfo("Could not read Webockets frame: payload missing / partial")
				                                     << errcodeInfoDef());
			}
		}

		result.message.append((const char*)block, (size_t)bytesRead);
		length -= bytesRead;
	}

	if (isMasked) {
		for (size_t i = 0; i < result.message.length(); i++) {
			result.message[i] ^= mask[i % 4];
		}
	}
	else {
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("Messages from the client MUST be masked!"));
	}

	return result;
}

boost::optional<WebsocketsFrame> getWebsocketsFrameTimeout(int clientSocket, int timeoutMs) {
	pollfd pfd;
	pfd.fd = clientSocket;
	pfd.events = POLLIN;
	pfd.revents = 0;

	int pollResult = poll(&pfd, 1, timeoutMs);
	if (pollResult == 1){
		return getWebsocketsFrame(clientSocket);
	}
	else if (pollResult < 0){
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("poll(): polling for Websockets frame")
		                                     << errcodeInfoDef());
	}
	else {
		return boost::optional<WebsocketsFrame>();
	}
}


void sendWebsocketsFrame(int clientSocket, WebsocketsFrame &frame){
	uint8_t firstByte = 0;
	uint64_t msgLen = frame.message.length();

	if (frame.isFin){
		firstByte |= 0x80;
	}

	firstByte |= (uint8_t)frame.opcode;
	if (write(clientSocket, &firstByte, 1) != 1){
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not write Websockets frame (first byte)")
		                                     << errcodeInfoDef());
	}
	if (msgLen < 126){
		uint8_t secondByte = (uint8_t) msgLen;
		if (write(clientSocket, &secondByte, 1) != 1){
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not write Websockets frame (length < 126)")
			                                     << errcodeInfoDef());
		}
	}
	else if (msgLen <= 0xFFFF){
		uint8_t secondByte = 126;
		uint16_t lenWord = htons((uint16_t)msgLen);
		if (write(clientSocket, &secondByte, 1) != 1 || write(clientSocket, &lenWord, 2) != 2){
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not write Websockets frame (126 <= length <= 65535")
			                                     << errcodeInfoDef());
		}
	}
	else {
		uint8_t secondByte = 126;
		uint64_t lenQword = htobe64(msgLen);
		if (write(clientSocket, &secondByte, 1) != 1 || write(clientSocket, &lenQword, 8) != 8){
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not write Websockets frame (length > 65535")
			                                     << errcodeInfoDef());
		}
	}

	const char* data = frame.message.c_str();
	const size_t blockSize = 65536;
	size_t lenLeft = frame.message.length();
	while (lenLeft > 0){
		size_t bytesToWrite = blockSize;
		if (lenLeft < bytesToWrite){
			bytesToWrite = lenLeft;
		}

		int bytesWritten = write(clientSocket, data, bytesToWrite);
		if (bytesWritten < 0){
			if (errno == EAGAIN || errno == EWOULDBLOCK){
				bytesWritten = 0;
			}
			else{
				BOOST_THROW_EXCEPTION(syscallError() << stringInfo("Could not read Webockets frame: payload missing / partial")
				                                     << errcodeInfoDef());
			}
		}
		lenLeft -= bytesWritten;
		data += bytesWritten;
	}
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

void respondWithObjectRef(int clientSocket, Response& response) {
	string responseData = response.getResponseHeaders();

	respondWithBuffer(clientSocket, responseData.c_str(), responseData.length());

	//DBG("getting first bodyNext");
	const string* bodyNext = response.getBodyNext();
	while(bodyNext != NULL){
		//DBG_FMT("sending body: string ptr is %p, body is %p, len is %d", bodyNext, bodyNext->c_str(), bodyNext->length());

		respondWithBuffer(clientSocket, bodyNext->c_str(), bodyNext->length());
		//DBG("bodyNext sent, getting next");
		bodyNext = response.getBodyNext();

	}
}

void respondWithObject(int clientSocket, Response response){
	respondWithObjectRef(clientSocket, response);
}


void respondWithCString(int clientSocket, const char* response) {
	respondWithBuffer(clientSocket, response, strlen(response));
}


void respondWithBuffer(int clientSocket, const char* response, size_t size) {
	//DBG("inRespondWithBuffer");
	size_t lenLeft = size;
	const size_t maxBlockSize = 65536;

	while (lenLeft > 0) {
		size_t lenCurrent = min(maxBlockSize, lenLeft);
		//DBG_FMT("sending buffer one, %d bytes from ptr %p", lenCurrent, response);
		if (write(clientSocket, response, lenCurrent) != (int)lenCurrent) {
			if (errno == EAGAIN || errno == EWOULDBLOCK){
				lenCurrent = 0;
				//DBG("eagain/wouldblock");
			}
			else{
				BOOST_THROW_EXCEPTION(networkError() << stringInfo("respondWith: could not send response.") << errcodeInfoDef());
			}
		}
		response += lenCurrent;
		lenLeft -= lenCurrent;
	}
}




