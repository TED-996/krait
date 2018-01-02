#include "networkManager.h"
#include "except.h"
#include "openSslServerSocket.h"
#include "serverSocket.h"


struct pollfd getPollFd(int socket) {
    struct pollfd result;
    result.fd = socket;
    result.events = POLLIN;
    result.revents = 0;

    return result;
}


NetworkManager::NetworkManager(
    boost::optional<u_int16_t> httpPort, boost::optional<u_int16_t> httpsPort, const Config& config)
        : httpSocket(nullptr), httpsSocket(nullptr) {
    if (httpPort != boost::none) {
        httpSocket = std::make_unique<ServerSocket>(ServerSocket::fromAnyOnPort(httpPort.get()));
        httpSocket->initialize();
        pollFds.push_back(getPollFd(httpSocket->getFd()));
        sockets.push_back(std::ref(*httpSocket));
    }
    if (httpsPort != boost::none) {
        httpsSocket = std::make_unique<SslServerSocket>(SslServerSocket::fromAnyOnPort(httpsPort.get(), config));
        httpsSocket->initialize();
        pollFds.push_back(getPollFd(httpsSocket->getFd()));
        sockets.push_back(std::ref(*httpsSocket));
    }

    if (httpPort == boost::none && httpsPort == boost::none) {
        BOOST_THROW_EXCEPTION(configError() << stringInfo("No HTTP/HTTPS ports specified, please set at least one."));
    }
}


void NetworkManager::listen(size_t backlog) {
    if (httpSocket != nullptr) {
        httpSocket->listen(backlog);
    }
    if (httpsSocket != nullptr) {
        httpsSocket->listen(backlog);
    }
}

std::unique_ptr<IManagedSocket> NetworkManager::acceptTimeout(int timeoutMs) {
    if (sockets.size() == 0) {
        BOOST_THROW_EXCEPTION(serverError() << stringInfo("NetworkManager accept called without any open sockets."));
    }

    for (auto& it : pollFds) {
        it.revents = 0;
    }

    int pollResult = poll(pollFds.data(), pollFds.size(), timeoutMs);
    if (pollResult < 0) {
        BOOST_THROW_EXCEPTION(
            networkError() << stringInfo("poll(): waiting to accept new client.") << errcodeInfoDef());
    }
    if (pollResult == 0) {
        return nullptr;
    }

    for (int i = 0; i < pollFds.size(); i++) {
        if (pollFds[i].revents == POLLIN) {
            return sockets[i].get().accept();
        }
    }

    BOOST_THROW_EXCEPTION(
        serverError() << stringInfo("NetworkManager poll returned existing clients, but no accept() was found."));
}

void NetworkManager::close() {
    httpSocket.reset();
    httpsSocket.reset();

    pollFds.clear();
    sockets.clear();
}
