#include "noSslServerSocket.h"

#if SSL_LIB == 0

#include "except.h"

void NoSslServerSocket::throwErrorNoSsl() {
    BOOST_THROW_EXCEPTION(
        configError() << stringInfo("Krait built without SSL support. Please use only HTTP or use a build with SSL."));
}

#endif
