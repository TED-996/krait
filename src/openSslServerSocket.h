#pragma once

#if SSL_LIB == 1

#include "IServerSocket.h"
#include "config.h"
#include <openssl/ssl.h>


class OpenSslServerSocket : public IServerSocket {
    class OpenSslInitStruct {
    private:
        OpenSslInitStruct();

    public:
        ~OpenSslInitStruct();

        static void ensureInit();
    };

    static const int InvalidSocket = -1;

    int socket;
    bool listening;

    const Config& config;
    SSL_CTX* ctx;

public:
    OpenSslServerSocket(const Config& config, int socket);
    OpenSslServerSocket(OpenSslServerSocket&) = delete;
    OpenSslServerSocket(OpenSslServerSocket&& other) noexcept;
    ~OpenSslServerSocket();

    OpenSslServerSocket& operator=(const OpenSslServerSocket& other) = delete;
    OpenSslServerSocket& operator=(OpenSslServerSocket&& other) noexcept;

    void initialize() override;
    int getFd() override;
    bool listen(size_t backlog) override;
    std::unique_ptr<IManagedSocket> accept() override;
    std::unique_ptr<IManagedSocket> acceptTimeout(int timeoutMs) override;

    static OpenSslServerSocket fromAnyOnPort(uint16_t port, const Config& config);
};

typedef OpenSslServerSocket SslServerSocket;

#endif