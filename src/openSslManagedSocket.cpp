#include "openSslManagedSocket.h"
#include "raiiAlarm.h"


#if SSL_LIB == 1

#include <sys/poll.h>
#include <openssl/rand.h>
#include "except.h"
#include "sslUtils.h"
#include "logger.h"
#include "dbgStopwatch.h"
#include "v2HttpParser.h"


OpenSslManagedSocket::OpenSslManagedSocket(int socket, SSL_CTX* ctx)
	: ManagedSocket(socket), ssl(SSL_new(ctx)) {
	SSL_set_fd(ssl, socket);
}

OpenSslManagedSocket::~OpenSslManagedSocket() {
	if (ssl != nullptr) {
		SSL_free(ssl);
	}
}

void OpenSslManagedSocket::initialize() {
	if (ssl == nullptr) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("OpenSslManagedSocket::initialize: Managed socket used after context detach."));
	}
	
	if (SSL_accept(ssl) <= 0) {
		BOOST_THROW_EXCEPTION(sslError() << stringInfo("SSL_accept: performing SSL handshake") << SslUtils::getSslErrorInfo());
	}
}

void OpenSslManagedSocket::atFork() {
	if (ssl == nullptr) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("OpenSslManagedSocket::atFork: Managed socket used after context detach."));
	}

	RAND_poll();
}

void OpenSslManagedSocket::detachContext() {
	ssl = nullptr;
}

int OpenSslManagedSocket::write(const void* data, size_t nBytes, int timeoutSeconds, bool* shouldRetry) {
	if (ssl == nullptr) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("OpenSslManagedSocket::write: Managed socket used after context detach."));
	}

	int bytesWritten;
	*shouldRetry = false;

	if (timeoutSeconds == -1) {
		bytesWritten = SSL_write(ssl, data, nBytes);
	}
	else {
		RaiiAlarm alarm(timeoutSeconds, true);
		bytesWritten = SSL_write(ssl, data, nBytes);

		if (alarm.isFinished()) {
			return -1;
		}
	}
	*shouldRetry = (bytesWritten < 0 && BIO_should_retry(SSL_get_wbio(ssl)));
	return bytesWritten;
}

int OpenSslManagedSocket::read(void* destination, size_t nBytes, int timeoutSeconds, bool* shouldRetry) {
	if (ssl == nullptr) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("OpenSslManagedSocket::read: Managed socket used after context detach."));
	}

	int bytesRead;
	*shouldRetry = false;

	if (timeoutSeconds == -1) {
		bytesRead = SSL_read(ssl, destination, nBytes);
	}
	else {
		RaiiAlarm alarm(timeoutSeconds, true);
		bytesRead = SSL_read(ssl, destination, nBytes);

		if (alarm.isFinished()) {
			return -1;
		}
	}
	*shouldRetry = (bytesRead < 0 && BIO_should_retry(SSL_get_rbio(ssl)));
	return bytesRead;
	
	//return SSL_read(ssl, destination, nBytes);
}

#endif