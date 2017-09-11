#pragma once
#include "INetworkManager.h"

class NetworkManager : public INetworkManager
{
private:
	static const int InvalidSocket = -1;

	int socket;
	bool listening;
public:
	explicit NetworkManager(int socket);
	NetworkManager(NetworkManager&) = delete;
	NetworkManager(NetworkManager&& source) noexcept;
	static NetworkManager fromAnyOnPort(short port);
	~NetworkManager() override;

	void initialize() override;

	int getFd() override;
	bool listen(size_t backlog = -1) override;
	std::unique_ptr<IManagedSocket> accept() override;
	std::unique_ptr<IManagedSocket> acceptTimeout(int timeoutMs) override;
};
