#pragma once

#if SSL_LIB == 1

#include <openssl/ssl.h>
#include "managedSocket.h"

class OpenSslManagedSocket : public ManagedSocket
{
private:
	SSL* ssl;

	int write(const void* data, size_t nBytes) override;
	int read(void* destination, size_t nBytes) override;
public:
	OpenSslManagedSocket(int socket, SSL_CTX* ctx);

	void initialize() override;
	std::unique_ptr<Request> getRequestTimeout(int timeoutMs) override;
};

#endif