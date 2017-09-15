#pragma once
#include <memory>
#include "IServerSocket.h"

class NetworkManager
{
	std::unique_ptr<IServerSocket> httpSocket;
	std::unique_ptr<IServerSocket> httpsSocket;
public:
};
