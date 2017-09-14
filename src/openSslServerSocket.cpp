#include "openSslServerSocket.h"

#if SSL_LIB == 1

#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/poll.h>
#include "except.h"


std::string getOpenSslErrors();


OpenSslServerSocket::OpenSslInitStruct::OpenSslInitStruct() {
	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();
}

OpenSslServerSocket::OpenSslInitStruct::~OpenSslInitStruct() {
	EVP_cleanup();
}

void OpenSslServerSocket::OpenSslInitStruct::ensureInit() {
	static OpenSslInitStruct raiiInstance;
	(void)raiiInstance;
}

OpenSslServerSocket::OpenSslServerSocket(Config& config, int socket)
	: socket(socket), config(config) {
	listening = false;
	ctx = nullptr;

	OpenSslInitStruct::ensureInit();
}

OpenSslServerSocket::OpenSslServerSocket(OpenSslServerSocket&& other) noexcept
	: config(other.config) {
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

void OpenSslServerSocket::initialize() {
	const SSL_METHOD *method = SSLv23_server_method();

	ctx = SSL_CTX_new(method);
	if (!ctx) {
		BOOST_THROW_EXCEPTION(sslError() << stringInfo("SSL_CTX_new failed") << sslErrorInfo(getOpenSslErrors()));
	}

#ifdef SSL_CTX_set_ecdh_auto
	SSL_CTX_set_ecdh_auto(ctx, 1);
#endif

	//TODO: configure cert + key
	if (config.getCertFilename() == boost::none) {
		BOOST_THROW_EXCEPTION(serverError()
			<< stringInfo("OpenSslServerSocket initialied WITHOUT krait.config.ssl_certificate_path being configured."));
	}
	
	if (SSL_CTX_use_certificate_file(ctx, config.getCertFilename().get().c_str(), SSL_FILETYPE_PEM) <= 0) {
		BOOST_THROW_EXCEPTION(sslError() << stringInfo("SSL_CTX_use_certificate_file failed") << sslErrorInfo(getOpenSslErrors()));
	}

	if (config.getCertKeyFilename() != boost::none) {
		if (SSL_CTX_use_PrivateKey_file(ctx, config.getCertKeyFilename().get().c_str(), SSL_FILETYPE_PEM) <= 0) {
			BOOST_THROW_EXCEPTION(sslError() << stringInfo("SSL_CTX_use_PrivateKey_file failed") << sslErrorInfo(getOpenSslErrors()));
		}
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
		return true; //Make it idempotent.
	}

	this->listening = (::listen(this->socket, (int)backlog) == 0);
	if (!this->listening) {
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("listen(): Setting OpenSSL socket to listening.") << errcodeInfoDef());
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
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("accept(): accepting new socket in OpenSslServerSocket"));
	}
	//return std::make_unique<OpenSslManagedSocket>(newSocket);
	return nullptr;
}

std::unique_ptr<IManagedSocket> OpenSslServerSocket::acceptTimeout(int timeoutMs) {
	if (socket == InvalidSocket) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("OpenSslServerSocket used (acceptTimeout) after deconstruction."));
	}
	if (!listening) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("OpenSslServerSocket not listening before call to acceptTimeout()"));
	}

	pollfd pfd;
	pfd.fd = this->socket;
	pfd.events = POLLIN;
	pfd.revents = 0;

	int pollResult = poll(&pfd, 1, timeoutMs);
	if (pollResult < 0) {
		BOOST_THROW_EXCEPTION(networkError() << stringInfo("poll(): waiting to accept new client."));
	}
	if (pollResult == 0) {
		return nullptr;
	}

	return accept();
}


int getOpenSslErrorsCallback(const char* str, size_t len, void* u);

std::string getOpenSslErrors() {
	std::string result;
	
	ERR_print_errors_cb(getOpenSslErrorsCallback, &result);

	return result;
}

int getOpenSslErrorsCallback(const char* str, size_t len, void* u) {
	std::string* result = (std::string*)u;
	result->append(str, len);

	return 1;
}
#endif
