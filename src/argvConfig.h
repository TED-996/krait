#pragma once
#include <boost/optional.hpp>
#include <string>

class ArgvConfig {
    std::string siteRoot;
    boost::optional<u_int16_t> httpPort;
    boost::optional<u_int16_t> httpsPort;
    bool isSingleProcess;

public:
    ArgvConfig(std::string siteRoot,
        boost::optional<u_int16_t> httpPort,
        boost::optional<u_int16_t> httpsPort,
        bool isSingleProcess)
            : siteRoot(std::move(siteRoot))
            , httpPort(httpPort)
            , httpsPort(httpsPort)
            , isSingleProcess(isSingleProcess) {
    }

    const std::string& getSiteRoot() const {
        return siteRoot;
    }

    const boost::optional<u_int16_t>& getHttpPort() const {
        return httpPort;
    }

    const boost::optional<u_int16_t>& getHttpsPort() const {
        return httpsPort;
    }

    bool getIsSingleProcess() const {
        return isSingleProcess;
    }
};
