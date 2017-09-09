#include "managedSocket.h"
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
#include "except.h"
#include "logger.h"
#include "v2HttpParser.h"

//#define DBG_DISABLE
#include"dbg.h"
#include "dbgStopwatch.h"


ManagedSocket::ManagedSocket(int socket) : socket(socket) {
}

ManagedSocket::ManagedSocket(ManagedSocket&& source) noexcept {
	this->socket = source.socket;
	source.socket = InvalidSocket;
}

int ManagedSocket::getFd() {
	return socket;
}

ManagedSocket::~ManagedSocket() {
	if (this->socket != InvalidSocket) {
		close(socket);
		this->socket = InvalidSocket;
	}
}

void ManagedSocket::initialize() {
}


int ManagedSocket::write(const void* data, size_t nBytes) {
	return ::write(this->socket, data, nBytes);
}

int ManagedSocket::read(void* destination, size_t nBytes) {
	return ::read(this->socket, destination, nBytes);
}

bool ManagedSocket::readExactly(void* destination, size_t nBytes) {
	while (nBytes > 0) {
		int bytesRead = read(destination, nBytes);
		//DBG_FMT("[readExactly] Read %1% bytes, errno is %2%", bytesRead, errno);

		if (bytesRead == 0) {
			return false;
		}

		if (bytesRead < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				bytesRead = 0;
			}
			else {
				return false;
			}
		}
		destination = (void*)((char*)destination + bytesRead);
		nBytes -= bytesRead;
	}
	return true;
}

std::unique_ptr<Request> ManagedSocket::getRequest() {
	static V2HttpParser parser(8192, 1024 * 1024 * 128);
	parser.reset();


	char buffer[4096];

	while(!parser.isFinished()) {
		int bytesRead = read(buffer, sizeof(buffer));

		if (bytesRead < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				bytesRead = 0;
			}
			else {
				BOOST_THROW_EXCEPTION(networkError() << stringInfo("read(): getting request from socket") << errcodeInfoDef());
			}
		}

		if (bytesRead != 0) {
			try {
				parser.consume(buffer, bytesRead);
			}
			catch (httpParseError err) {
				Loggers::logErr(formatString("Error parsing http request: %s\n", err.what()));
				BOOST_THROW_EXCEPTION(networkError() << stringInfo("Error parsing HTTP request"));
			}
		}
	}
	
	if (parser.isError()) {
		Loggers::logErr(formatString("Error parsing http request: Status code %1%, message %2%\n",
			parser.getErrorStatusCode(), parser.getErrorMessage()));
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("Error parsing HTTP request"));
	}

	return parser.getParsed();
}

void ManagedSocket::respondWithBuffer(const void* response, size_t size) {
	const void* ptr = response;
	const void* const end = (const void*)((const char*)response + size);

	while(ptr < end) {
		int bytesWritten = write(ptr, (const char*)end - (const char*)ptr);
		if (bytesWritten < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				bytesWritten = 0;
			}
			else {
				BOOST_THROW_EXCEPTION(networkError() << stringInfo("write(): sending response"));
			}
		}
		ptr = (void*)((const char*)ptr + bytesWritten);
	}
}

std::unique_ptr<Request> ManagedSocket::getRequestTimeout(int timeoutMs) {
	static V2HttpParser parser(8192, 1024 * 1024 * 128);
	parser.reset();

	char buffer[4096];
	int pollResult;

	pollfd pfd;
	pfd.fd = this->socket;
	pfd.events = POLLIN;
	pfd.revents = 0;

	DbgAggregatedStopwatch stopwatch("ManagedSocket::getRequestTimeout");

	while (!parser.isFinished() && (pollResult = poll(&pfd, 1, timeoutMs)) != 0) {
		if (pollResult < 0) {
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("poll(): waiting for request from socket."));
		}

		int bytesRead = read(buffer, sizeof(buffer));
		if (bytesRead == 0) {
			return nullptr;
		}
		if (bytesRead < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				bytesRead = 0;
			}
			else {
				BOOST_THROW_EXCEPTION(networkError() << stringInfo("read(): getting request from socket") << errcodeInfoDef());
			}
		}

		if (bytesRead != 0) {
			try {
				DbgDependentStopwatch networkStopwatch(stopwatch);
				parser.consume(buffer, bytesRead);
			}
			catch (httpParseError err) {
				Loggers::logErr(formatString("Error parsing http request: %s\n", err.what()));
				BOOST_THROW_EXCEPTION(networkError() << stringInfo("Error parsing HTTP request"));
			}
		}
	}

	if (parser.isError()) {
		Loggers::logErr(formatString("Error parsing http request: Status code %1%, message %2%\n",
			parser.getErrorStatusCode(), parser.getErrorMessage()));
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("Error parsing HTTP request"));
	}

	if (!parser.isFinished()) {
		return nullptr;
	}

	return parser.getParsed();
}

void ManagedSocket::respondWithObject(Response&& response) {
	std::string responseData = response.getResponseHeaders();

	respondWithBuffer(responseData.c_str(), responseData.length());

	const std::string* bodyNext = response.getBodyNext();
	while (bodyNext != NULL) {
		respondWithBuffer(bodyNext->c_str(), bodyNext->length());
		bodyNext = response.getBodyNext();
	}
}

WebsocketsFrame ManagedSocket::getWebsocketsFrame() {
	WebsocketsFrame result;

	uint8_t firstByte = 0;
	uint8_t secondByte = 0;
	uint64_t length;
	uint32_t mask[4] = { 0, 0, 0, 0 };

	if (!readExactly(&firstByte, 1) || !readExactly(&secondByte, 1)) {
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not read WebSockets frame: one of first 2 bytes missing.")
			<< errcodeInfoDef());
	}
	result.isFin = (bool)(firstByte & 0x80);
	result.opcode = (WebsocketsOpcode)(firstByte & 0xF);
	bool isMasked = (bool)(secondByte & 0x80);

	length = (uint64_t)(secondByte & 0x7F);
	if (length == 126) {
		uint16_t lenTmp = 0;
		if (!readExactly(&lenTmp, 2)) {
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not read Websockets frame: 16-bit length missing.")
				<< errcodeInfoDef());
		}
		length = ntohs(lenTmp);
	}
	else if (length == 127) {
		if (!readExactly(&length, 8)) {
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not read Websockets frame: 64-bit length missing.")
				<< errcodeInfoDef());
		}
		length = be64toh(length);
	}

	if (isMasked) {
		if (!readExactly(&mask[0], 1) ||
			!readExactly(&mask[1], 1) ||
			!readExactly(&mask[2], 1) ||
			!readExactly(&mask[3], 1)) {
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not read Websockets frame: mask missing.")
				<< errcodeInfoDef());
		}
	}
	unsigned char block[4096];

	while (length > 0) {
		size_t sizeToRead = sizeof(block);
		if (length < sizeToRead) {
			sizeToRead = (size_t)length;
		}
		int bytesRead = read(&block, sizeToRead);
		if (bytesRead < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				bytesRead = 0;
			}
			else {
				BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not read Webockets frame: payload missing / partial")
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

boost::optional<WebsocketsFrame> ManagedSocket::getWebsocketsFrameTimeout(int timeoutMs) {
	pollfd pfd;
	pfd.fd = this->socket;
	pfd.events = POLLIN;
	pfd.revents = 0;

	int pollResult = poll(&pfd, 1, timeoutMs);
	if (pollResult == 1) {
		return getWebsocketsFrame(); //FIXME: this is a blocking call. Slow Loris vuln.
	}
	else if (pollResult < 0) {
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("poll(): polling for Websockets frame")
			<< errcodeInfoDef());
	}
	else {
		return boost::optional<WebsocketsFrame>();
	}
}

void ManagedSocket::sendWebsocketsFrame(WebsocketsFrame& frame) {
	uint8_t firstByte = 0;
	uint64_t msgLen = frame.message.length();

	if (frame.isFin) {
		firstByte |= 0x80;
	}

	firstByte |= (uint8_t)frame.opcode;
	if (write(&firstByte, 1) != 1) {
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not write Websockets frame (first byte)")
			<< errcodeInfoDef());
	}
	if (msgLen < 126) {
		uint8_t secondByte = (uint8_t)msgLen;
		if (write(&secondByte, 1) != 1) {
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not write Websockets frame (length < 126)")
				<< errcodeInfoDef());
		}
	}
	else if (msgLen <= 0xFFFF) {
		uint8_t secondByte = 126;
		uint16_t lenWord = htons((uint16_t)msgLen);
		if (write(&secondByte, 1) != 1 || write(&lenWord, 2) != 2) {
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not write Websockets frame (126 <= length <= 65535")
				<< errcodeInfoDef());
		}
	}
	else {
		uint8_t secondByte = 126;
		uint64_t lenQword = htobe64(msgLen);
		if (write(&secondByte, 1) != 1 || write(&lenQword, 8) != 8) {
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not write Websockets frame (length > 65535")
				<< errcodeInfoDef());
		}
	}

	const char* data = frame.message.c_str();
	const size_t blockSize = 65536;
	size_t lenLeft = frame.message.length();
	while (lenLeft > 0) {
		size_t bytesToWrite = blockSize;
		if (lenLeft < bytesToWrite) {
			bytesToWrite = lenLeft;
		}

		int bytesWritten = write(data, bytesToWrite);
		if (bytesWritten < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				bytesWritten = 0;
			}
			else {
				BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not read Webockets frame: payload missing / partial")
					<< errcodeInfoDef());
			}
		}
		lenLeft -= bytesWritten;
		data += bytesWritten;
	}
}
