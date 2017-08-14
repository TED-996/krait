#pragma once
#include <memory>
#include "IManagedSocket.h"

class INetworkManager
{
public:
	virtual ~INetworkManager() = default;
	
	virtual void initialize() = 0;

	virtual int getFd() = 0;

	virtual bool listen(size_t backlog) = 0;
	virtual std::unique_ptr<IManagedSocket> accept() = 0;
	virtual std::unique_ptr<IManagedSocket> acceptTimeout(int timeoutMs) = 0;
};
