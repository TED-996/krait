#pragma once

#if SSL_LIB == 1

#include "managedSocket.h"
#include <openssl/ssl.h>

class OpenSslManagedSocket : public ManagedSocket {
private:
    SSL* ssl;

    int write(const void* data, size_t nBytes, int timeoutSeconds, bool* shouldRetry) override;
    int read(void* destination, size_t nBytes, int timeoutSeconds, bool* shouldRetry) override;

public:
    OpenSslManagedSocket(int socket, SSL_CTX* ctx);
    ~OpenSslManagedSocket();

    void initialize() override;
    void atFork() override;
    void detachContext() override;
};

#endif