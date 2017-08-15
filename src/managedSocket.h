#pragma once
#include "IManagedSocket.h"

class ManagedSocket : public IManagedSocket
{
private:
	static const int InvalidSocket = -1;

	int socket;

	int write(const void* data, size_t nBytes) override;
	int read(void* destination, size_t nBytes) override;

	void respondWithBuffer(const void* response, size_t size);
	bool readExactly(void* destination, size_t nBytes);

	int getFd() override;
public:
	explicit ManagedSocket(int socket);
	ManagedSocket(ManagedSocket&) = delete;
	ManagedSocket(ManagedSocket&& source) noexcept;

	~ManagedSocket() override;

	void initialize() override;
	std::unique_ptr<Request> getRequest() override;
	std::unique_ptr<Request> getRequestTimeout(int timeoutMs) override;
	WebsocketsFrame getWebsocketsFrame() override;
	boost::optional<WebsocketsFrame> getWebsocketsFrameTimeout(int timeoutMs) override;
	void sendWebsocketsFrame(WebsocketsFrame& frame) override;
	void respondWithObject(Response&& response) override;
};
