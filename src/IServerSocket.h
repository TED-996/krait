#pragma once
#include <memory>
#include "IManagedSocket.h"

class IServerSocket
{
public:
	virtual ~IServerSocket() = default;
	
	virtual void initialize() = 0;

	virtual int getFd() = 0;

	virtual bool listen(size_t backlog = -1) = 0;
	virtual std::unique_ptr<IManagedSocket> accept() = 0;
	virtual std::unique_ptr<IManagedSocket> acceptTimeout(int timeoutMs) = 0;
};
