#pragma once
#include<cstddef>
#include <boost/optional/optional.hpp>
#include "request.h"
#include "websocketsTypes.h"
#include "pageResponseRenderer.h"

class IManagedSocket
{
private:
	virtual int write(const void* data, size_t nBytes) = 0;
	virtual int read(void* destination, size_t nBytes) = 0;

	virtual int getFd() = 0;
public:
	virtual ~IManagedSocket() = default;

	virtual void initialize() = 0;
	virtual Request getRequest() = 0;
	virtual boost::optional<Request> getRequestTimeout(int timeoutMs) = 0;

	virtual WebsocketsFrame getWebsocketsFrame() = 0;
	virtual boost::optional<WebsocketsFrame> getWebsocketsFrameTimeout(int timeoutMs) = 0;

	virtual void sendWebsocketsFrame(WebsocketsFrame& frame) = 0;
	virtual void respondWithObject(Response&& response) = 0;
};
