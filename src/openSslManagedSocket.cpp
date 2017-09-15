#include "openSslManagedSocket.h"

#if SSL_LIB == 1

#include "except.h"
#include "sslUtils.h"


OpenSslManagedSocket::OpenSslManagedSocket(int socket, SSL_CTX* ctx)
	: ManagedSocket(socket), ssl(SSL_new(ctx)) {
	SSL_set_fd(ssl, socket);
}

void OpenSslManagedSocket::initialize() {
	if (SSL_accept(ssl) <= 0) {
		BOOST_THROW_EXCEPTION(sslError() << stringInfo("SSL_accept: performing SSL handshake") << SslUtils::getSslErrorInfo());
	}
}

std::unique_ptr<Request> OpenSslManagedSocket::getRequestTimeout(int timeoutMs) {
	"TODO: alarm & siginterrupt: https ://stackoverflow.com/questions/18548849/read-system-call-doesnt-fail-when-an-alarm-signal-is-received"
}

int OpenSslManagedSocket::write(const void* data, size_t nBytes) {
	return SSL_write(ssl, data, nBytes);
}

int OpenSslManagedSocket::read(void* destination, size_t nBytes) {
	return SSL_read(ssl, destination, nBytes);
}

#endif