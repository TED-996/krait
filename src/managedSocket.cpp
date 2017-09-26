#include "managedSocket.h"
#include <algorithm>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <endian.h>
#include "except.h"
#include "logger.h"
#include "v2HttpParser.h"
#include "raiiAlarm.h"

#define DBG_DISABLE
#include"dbg.h"
#include "dbgStopwatch.h"


ManagedSocket::ManagedSocket(int socket) : socket(socket) {
}

ManagedSocket::ManagedSocket(ManagedSocket&& source) noexcept {
	this->socket = source.socket;
	source.socket = InvalidSocket;
}

ManagedSocket::~ManagedSocket() {
	if (this->socket != InvalidSocket) {
		close(socket);
		this->socket = InvalidSocket;
	}
}

void ManagedSocket::initialize() {
}

void ManagedSocket::atFork() {
}

void ManagedSocket::detachContext() {
}

int ManagedSocket::write(const void* data, size_t nBytes, int timeoutSeconds, bool* shouldRetry) {
	int bytesWritten;
	*shouldRetry = false;

	if (timeoutSeconds == -1) {
		bytesWritten = ::write(this->socket, data, nBytes);
	}
	else {
		RaiiAlarm alarm(timeoutSeconds, true);
		bytesWritten = ::write(this->socket, data, nBytes);
		
		if (alarm.isFinished()) {
			return -1;
		}
	}
	*shouldRetry = (bytesWritten < 0 && (errno == EAGAIN || errno == EWOULDBLOCK));
	return bytesWritten;
}

int ManagedSocket::read(void* destination, size_t nBytes, int timeoutSeconds, bool* shouldRetry) {
	int bytesRead;
	*shouldRetry = false;

	if (timeoutSeconds == -1) {
		bytesRead = ::read(this->socket, destination, nBytes);
	}
	else {
		RaiiAlarm alarm(timeoutSeconds, true);
		bytesRead = ::read(this->socket, destination, nBytes);

		if (alarm.isFinished()) {
			return -1;
		}
	}
	*shouldRetry = (bytesRead < 0 && (errno == EAGAIN || errno == EWOULDBLOCK));
	return bytesRead;
}

bool ManagedSocket::readExactly(void* destination, size_t nBytes) {
	while (nBytes > 0) {
		bool shouldRetry;
		int bytesRead = read(destination, nBytes, -1, &shouldRetry);
		//DBG_FMT("[readExactly] Read %1% bytes, errno is %2%", bytesRead, errno);

		if (shouldRetry) {
			continue;
		}
		if (bytesRead <= 0) {
			return false;
		}

		destination = (void*)((char*)destination + bytesRead);
		nBytes -= bytesRead;
	}
	return true;
}

bool ManagedSocket::writeExactly(void* data, size_t nBytes) {
	while (nBytes > 0) {
		bool shouldRetry;
		int bytesWritten = write(data, nBytes, -1, &shouldRetry);
		//DBG_FMT("[readExactly] Read %1% bytes, errno is %2%", bytesRead, errno);

		if (shouldRetry) {
			continue;
		}
		if (bytesWritten <= 0) {
			return false;
		}

		data = (void*)((char*)data + bytesWritten);
		nBytes -= bytesWritten;
	}
	return true;
}

std::unique_ptr<Request> ManagedSocket::getRequest() {
	static V2HttpParser parser(8192, 1024 * 1024 * 128);
	parser.reset();


	char buffer[4096];

	while(!parser.isFinished()) {
		bool shouldRetry;
		int bytesRead = read(buffer, sizeof(buffer), -1, &shouldRetry);

		if (shouldRetry) {
			continue;
		}

		if (bytesRead <= 0) {
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("read(): getting request from socket") << errcodeInfoDef());
		}

		try {
			parser.consume(buffer, bytesRead);
		}
		catch (httpParseError err) {
			Loggers::logErr(formatString("Error parsing http request: %s\n", err.what()));
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("Error parsing HTTP request"));
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
	DbgStopwatch stopwatch("ManagedSocket::respondWithBuffer");
	(void)stopwatch;

	const void* ptr = response;
	const void* const end = (const void*)((const char*)response + size);

	while(ptr < end) {
		bool shouldRetry;
		int bytesWritten = write(ptr, (const char*)end - (const char*)ptr, -1, &shouldRetry);
		
		if (shouldRetry) {
			continue;
		}

		if (bytesWritten <= 0) {
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("write(): sending response"));
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

		bool shouldRetry;
		//TODO FIXME: this timeout is always too large (up to a **doubling** of the allowed timeout)
		int bytesRead = read(buffer, sizeof(buffer), timeoutMs, &shouldRetry);

		if (shouldRetry) {
			continue;
		}

		if (bytesRead <= 0) {
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("read(): getting request from socket") << errcodeInfoDef());
		}

		try {
			DbgDependentStopwatch networkStopwatch(stopwatch);
			parser.consume(buffer, bytesRead);
		}
		catch (httpParseError err) {
			Loggers::logErr(formatString("Error parsing HTTP request: %s\n", err.what()));
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("Error parsing HTTP request"));
		}
	}

	if (parser.isError()) {
		Loggers::logErr(formatString("Error parsing HTTP request: Status code %1%, message %2%\n",
			parser.getErrorStatusCode(), parser.getErrorMessage()));
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("Error parsing HTTP request"));
	}

	if (!parser.isFinished()) {
		return nullptr;
	}

	return parser.getParsed();
}

void ManagedSocket::respondWithObject(Response&& response) {
	DbgStopwatch stopwatch("Sending response");
	(void)stopwatch;

	{
		DbgStopwatch inner("Sending headers");
		(void)inner;

		std::string responseData = response.getResponseHeaders();

		respondWithBuffer(responseData.c_str(), responseData.length());

		inner.stopPrint();
	}

	{
		DbgStopwatch inner("Sending body");
		(void)inner;

		const std::string* bodyNext = response.getBodyNext();
		while (bodyNext != nullptr) {
			respondWithBuffer(bodyNext->c_str(), bodyNext->length());
			bodyNext = response.getBodyNext();
		}

		inner.stopPrint();
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
		bool shouldRetry;

		int bytesRead = read(&block, sizeToRead, -1, &shouldRetry);

		if (shouldRetry) {
			continue;
		}

		if (bytesRead <= 0) {
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not read Webockets frame: payload missing / partial")
				<< errcodeInfoDef());
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
	if (!writeExactly(&firstByte, 1)) {
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not write Websockets frame (first byte)")
			<< errcodeInfoDef());
	}
	if (msgLen < 126) {
		uint8_t secondByte = (uint8_t)msgLen;
		if (!writeExactly(&secondByte, 1)) {
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not write Websockets frame (length < 126)")
				<< errcodeInfoDef());
		}
	}
	else if (msgLen <= 0xFFFF) {
		uint8_t secondByte = 126;
		uint16_t lenWord = htons((uint16_t)msgLen);
		if (!writeExactly(&secondByte, 1) || !writeExactly(&lenWord, 2)) {
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not write Websockets frame (126 <= length <= 65535")
				<< errcodeInfoDef());
		}
	}
	else {
		uint8_t secondByte = 126;
		uint64_t lenQword = htobe64(msgLen);
		if (!writeExactly(&secondByte, 1) || !writeExactly(&lenQword, 8)) {
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

		bool shouldRetry;
		int bytesWritten = write(data, bytesToWrite, -1, &shouldRetry);

		if (shouldRetry) {
			continue;
		}

		if (bytesWritten <= 0) {
			BOOST_THROW_EXCEPTION(networkError() << stringInfo("Could not read Webockets frame: payload missing / partial")
				<< errcodeInfoDef());
		}
		lenLeft -= bytesWritten;
		data += bytesWritten;
	}
}
