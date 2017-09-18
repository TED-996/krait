#pragma once

#if SSL_LIB == 1

#include <openssl/ssl.h>
#include "managedSocket.h"

class OpenSslManagedSocket : public ManagedSocket
{
private:
	SSL* ssl;

	int write(const void* data, size_t nBytes, int timeoutSeconds, bool* shouldRetry) override;
	int read(void* destination, size_t nBytes, int timeoutSeconds, bool* shouldRetry) override;
public:
	OpenSslManagedSocket(int socket, SSL_CTX* ctx);
	~OpenSslManagedSocket();

	void initialize() override;
};

#endif