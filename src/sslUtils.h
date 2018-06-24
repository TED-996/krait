#pragma once
#include "except.h"

class SslUtils {
public:
    static sslErrorInfo getSslErrorInfo() {
        return sslErrorInfo(getSslErrors());
    }

    static std::string getSslErrors();
};
