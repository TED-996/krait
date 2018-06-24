#pragma once
#include "IManagedSocket.h"

class ManagedSocket : public IManagedSocket {
private:
    static const int InvalidSocket = -1;

    int socket;

    int write(const void* data, size_t nBytes, int timeoutSeconds, bool* shouldRetry) override;
    int read(void* destination, size_t nBytes, int timeoutSeconds, bool* shouldRetry) override;

    void respondWithBuffer(const void* response, size_t size);
    bool readExactly(void* destination, size_t nBytes);
    bool writeExactly(void* data, size_t nBytes);


public:
    explicit ManagedSocket(int socket);
    ManagedSocket(ManagedSocket&) = delete;
    ManagedSocket(ManagedSocket&& source) noexcept;

    ~ManagedSocket() override;

    int getFd() override {
        return socket;
    }

    void initialize() override;
    void atFork() override;
    void detachContext() override;

    std::unique_ptr<Request> getRequest() override;
    std::unique_ptr<Request> getRequestTimeout(int timeoutMs) override;
    WebsocketsFrame getWebsocketsFrame() override;
    boost::optional<WebsocketsFrame> getWebsocketsFrameTimeout(int timeoutMs) override;
    void sendWebsocketsFrame(WebsocketsFrame& frame) override;
    void respondWithObject(Response&& response) override;
};
