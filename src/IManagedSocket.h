#pragma once
#include <memory>
#include "request.h"
#include "websocketsTypes.h"
#include "pageResponseRenderer.h"

class IManagedSocket
{
private:
	virtual int write(const void* data, size_t nBytes, int timeoutSeconds, bool* shouldRetry) = 0;
	virtual int read(void* destination, size_t nBytes, int timeoutSeconds, bool* shouldRetry) = 0;

public:
	virtual ~IManagedSocket() = default;

	virtual int getFd() = 0;

	virtual void initialize() = 0;
	virtual std::unique_ptr<Request> getRequest() = 0;
	virtual std::unique_ptr<Request> getRequestTimeout(int timeoutMs) = 0;

	virtual WebsocketsFrame getWebsocketsFrame() = 0;
	virtual boost::optional<WebsocketsFrame> getWebsocketsFrameTimeout(int timeoutMs) = 0;

	virtual void sendWebsocketsFrame(WebsocketsFrame& frame) = 0;
	virtual void respondWithObject(Response&& response) = 0;
};
