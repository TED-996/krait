#include "openSslServerSocket.h"

#if SSL_LIB == 1

#include "except.h"
#include "openSslManagedSocket.h"
#include "sslUtils.h"
#include <arpa/inet.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/poll.h>
#include <sys/socket.h>


OpenSslServerSocket::OpenSslInitStruct::OpenSslInitStruct() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

OpenSslServerSocket::OpenSslInitStruct::~OpenSslInitStruct() {
    EVP_cleanup();
}

void OpenSslServerSocket::OpenSslInitStruct::ensureInit() {
    static OpenSslInitStruct raiiInstance;
    (void) raiiInstance;
}

OpenSslServerSocket::OpenSslServerSocket(const Config& config, int socket) : socket(socket), config(config) {
    listening = false;
    ctx = nullptr;

    OpenSslInitStruct::ensureInit();
}

OpenSslServerSocket::OpenSslServerSocket(OpenSslServerSocket&& other) noexcept : config(other.config) {
    this->socket = other.socket;
    this->listening = other.listening;
    this->ctx = other.ctx;

    other.socket = InvalidSocket;
    other.listening = false;
    other.ctx = nullptr;
}

OpenSslServerSocket::~OpenSslServerSocket() {
    if (this->socket != InvalidSocket) {
        ::close(socket);
        this->socket = InvalidSocket;
    }

    this->listening = false;
    if (this->ctx != nullptr) {
        SSL_CTX_free(this->ctx);
        this->ctx = nullptr;
    }
}

OpenSslServerSocket& OpenSslServerSocket::operator=(OpenSslServerSocket&& other) noexcept {
    if (this->socket != InvalidSocket) {
        ::close(socket);
    }

    if (this->ctx != nullptr) {
        SSL_CTX_free(this->ctx);
    }

    this->socket = other.socket;
    this->listening = other.listening;
    this->ctx = other.ctx;

    other.socket = InvalidSocket;
    other.listening = false;
    other.ctx = nullptr;

    return *this;
}

int sslPemPasswdCb(char* buf, int size, int rwflag, void* password) {
    strncpy(buf, (const char*) (password), size);
    buf[size - 1] = '\0';
    return (strlen(buf));
}


void OpenSslServerSocket::initialize() {
    const SSL_METHOD* method = SSLv23_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
        BOOST_THROW_EXCEPTION(sslError() << stringInfo("SSL_CTX_new failed") << SslUtils::getSslErrorInfo());
    }

#ifdef SSL_CTX_set_ecdh_auto
    SSL_CTX_set_ecdh_auto(ctx, 1);
#endif

    if (config.getCertFilename() == boost::none) {
        BOOST_THROW_EXCEPTION(
            serverError() << stringInfo(
                "OpenSslServerSocket initialied WITHOUT krait.config.ssl_certificate_path being configured."));
    }
    if (config.getCertKeyFilename() == boost::none) {
        BOOST_THROW_EXCEPTION(
            serverError() << stringInfo(
                "OpenSSLServerSocket initialized WITHOUT krait.config.ssl_private_key_path being configured."));
    }

    if (SSL_CTX_use_certificate_file(ctx, config.getCertFilename().get().c_str(), SSL_FILETYPE_PEM) <= 0) {
        BOOST_THROW_EXCEPTION(
            sslError() << stringInfo("SSL_CTX_use_certificate_file failed") << SslUtils::getSslErrorInfo());
    }

    if (config.getCertKeyPassphrase() != boost::none) {
        SSL_CTX_set_default_passwd_cb(ctx, sslPemPasswdCb);
        SSL_CTX_set_default_passwd_cb_userdata(
            ctx, const_cast<void*>(static_cast<const void*>(config.getCertKeyPassphrase().get().c_str())));
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, config.getCertKeyFilename().get().c_str(), SSL_FILETYPE_PEM) <= 0) {
        BOOST_THROW_EXCEPTION(
            sslError() << stringInfo("SSL_CTX_use_PrivateKey_file failed") << SslUtils::getSslErrorInfo());
    }
}

bool OpenSslServerSocket::listen(size_t backlog) {
    if (backlog == -1) {
        backlog = SOMAXCONN;
    }

    if (this->socket == InvalidSocket) {
        BOOST_THROW_EXCEPTION(serverError() << stringInfo("OpenSslServerSocket used (listen) after deconstruction."));
    }
    if (listening) {
        return true; // Make it idempotent.
    }

    this->listening = (::listen(this->socket, (int) backlog) == 0);
    if (!this->listening) {
        BOOST_THROW_EXCEPTION(
            syscallError() << stringInfo("listen(): Setting OpenSSL socket to listening.") << errcodeInfoDef());
    }
    return this->listening;
}

int OpenSslServerSocket::getFd() {
    return socket;
}

std::unique_ptr<IManagedSocket> OpenSslServerSocket::accept() {
    if (socket == InvalidSocket) {
        BOOST_THROW_EXCEPTION(serverError() << stringInfo("OpenSslServerSocket used (accept) after deconstruction."));
    }
    if (!listening) {
        BOOST_THROW_EXCEPTION(serverError() << stringInfo("OpenSslServerSocket not listening before call to accept()"));
    }

    int newSocket = ::accept(this->socket, nullptr, nullptr);
    if (newSocket == -1) {
        BOOST_THROW_EXCEPTION(
            networkError() << stringInfo("accept(): accepting new socket in OpenSslServerSocket") << errcodeInfoDef());
    }
    return std::make_unique<OpenSslManagedSocket>(newSocket, ctx);
}

std::unique_ptr<IManagedSocket> OpenSslServerSocket::acceptTimeout(int timeoutMs) {
    if (socket == InvalidSocket) {
        BOOST_THROW_EXCEPTION(
            serverError() << stringInfo("OpenSslServerSocket used (acceptTimeout) after deconstruction."));
    }
    if (!listening) {
        BOOST_THROW_EXCEPTION(
            serverError() << stringInfo("OpenSslServerSocket not listening before call to acceptTimeout()"));
    }

    pollfd pfd;
    pfd.fd = this->socket;
    pfd.events = POLLIN;
    pfd.revents = 0;

    int pollResult = poll(&pfd, 1, timeoutMs);
    if (pollResult < 0) {
        BOOST_THROW_EXCEPTION(
            networkError() << stringInfo("poll(): waiting to accept new client.") << errcodeInfoDef());
    }
    if (pollResult == 0) {
        return nullptr;
    }

    return accept();
}


int getOpenSslErrorsCallback(const char* str, size_t len, void* u);

std::string SslUtils::getSslErrors() {
    std::string result;

    ERR_print_errors_cb(getOpenSslErrorsCallback, &result);

    return result;
}

int getOpenSslErrorsCallback(const char* str, size_t len, void* u) {
    std::string* result = (std::string*) u;
    result->append(str, len);

    return 1;
}


OpenSslServerSocket OpenSslServerSocket::fromAnyOnPort(uint16_t port, const Config& config) {
    struct sockaddr_in serverSockaddr;
    memset(&serverSockaddr, 0, sizeof(serverSockaddr));

    serverSockaddr.sin_family = AF_INET;
    serverSockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverSockaddr.sin_port = htons(port);

    int sd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sd == -1) {
        BOOST_THROW_EXCEPTION(networkError()
            << stringInfo("getListenSocket: could not create socket in ServerSocket::fromAnyOnPort.")
            << errcodeInfoDef());
    }

    const int reuseAddr = 1;
    int enable = (int) reuseAddr;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
        BOOST_THROW_EXCEPTION(networkError()
            << stringInfo("getListenSocket: coult not set reuseAddr in ServerSocket::fromAnyOnPort.")
            << errcodeInfoDef());
    }

    if (bind(sd, (sockaddr*) &serverSockaddr, sizeof(sockaddr)) != 0) {
        BOOST_THROW_EXCEPTION(networkError()
            << stringInfo("getListenSocket: could not bind socket in ServerSocket::fromAnyOnPort.")
            << errcodeInfoDef());
    }

    return OpenSslServerSocket(config, sd);
}
#endif
