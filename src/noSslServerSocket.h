#pragma once

#if SSL_LIB == 0

#include "IServerSocket.h"
#include "config.h"


class NoSslServerSocket : public IServerSocket {
    static[[noreturn]] void throwErrorNoSsl();


public:
    NoSslServerSocket(const Config& config, int socket) {
        throwErrorNoSsl();
    }
    NoSslServerSocket(NoSslServerSocket&) = delete;
    NoSslServerSocket(NoSslServerSocket&& other) noexcept = default;

    void initialize() override {
        throwErrorNoSsl();
    }
    int getFd() override {
        throwErrorNoSsl();
        return 0;
    }
    bool listen(size_t backlog) override {
        throwErrorNoSsl();
        return false;
    }
    std::unique_ptr<IManagedSocket> accept() override {
        throwErrorNoSsl();
        return nullptr;
    }
    std::unique_ptr<IManagedSocket> acceptTimeout(int timeoutMs) override {
        throwErrorNoSsl();
        return nullptr;
    }

    static NoSslServerSocket fromAnyOnPort(uint16_t port, const Config& config) {
        throwErrorNoSsl();
        return NoSslServerSocket(config, -1);
    }
};

typedef NoSslServerSocket SslServerSocket;

#endif