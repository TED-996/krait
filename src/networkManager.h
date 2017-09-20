#pragma once
#include <memory>
#include "IServerSocket.h"
#include "config.h"
#include <sys/poll.h>

class NetworkManager
{
	std::unique_ptr<IServerSocket> httpSocket;
	std::unique_ptr<IServerSocket> httpsSocket;

	std::vector<pollfd> pollFds;
	std::vector<const std::unique_ptr<IServerSocket>&> sockets;

public:
	NetworkManager(boost::optional<u_int16_t> httpPort, boost::optional<u_int16_t> httpsPort, const Config& config);

	void listen(size_t backlog = -1);
	std::unique_ptr<IManagedSocket> acceptTimeout(int timeoutMs);
	void close();
};
