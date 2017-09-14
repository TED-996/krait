#pragma once
#include "IServerSocket.h"

class ServerSocket : public IServerSocket
{
private:
	static const int InvalidSocket = -1;

	int socket;
	bool listening;
public:
	explicit ServerSocket(int socket);
	ServerSocket(ServerSocket&) = delete;
	ServerSocket(ServerSocket&& source) noexcept;
	static ServerSocket fromAnyOnPort(u_int16_t port);
	~ServerSocket() override;

	ServerSocket& operator=(const ServerSocket& other) = delete;
	ServerSocket& operator=(ServerSocket&& other) noexcept;

	void initialize() override;

	int getFd() override;
	bool listen(size_t backlog = -1) override;
	std::unique_ptr<IManagedSocket> accept() override;
	std::unique_ptr<IManagedSocket> acceptTimeout(int timeoutMs) override;
};
