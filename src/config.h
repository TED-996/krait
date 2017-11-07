#pragma once
#include "argvConfig.h"
#include "regexList.h"
#include "routes.h"
#include <vector>

class Config {
private:
    ArgvConfig argvConfig;

    std::vector<Route> routes;
    RegexList noStoreTargets;
    RegexList privateTargets;
    RegexList publicTargets;
    RegexList longTermTargets;

    int maxAgeDefault;
    int maxAgeLongTerm;

    boost::optional<std::string> certFilename;
    boost::optional<std::string> certKeyFilename;
    boost::optional<std::string> certKeyPassphrase;

    void loadRoutes();
    void loadCacheConfig();
    void loadSslConfig();

public:
    explicit Config(const ArgvConfig& argvConfig);

    const std::string& getSiteRoot() const {
        return argvConfig.getSiteRoot();
    }

    const boost::optional<u_int16_t>& getHttpPort() const {
        return argvConfig.getHttpPort();
    }

    const boost::optional<u_int16_t>& getHttpsPort() const {
        return argvConfig.getHttpsPort();
    }

    bool getIsSingleProcess() const {
        return argvConfig.getIsSingleProcess();
    }

    const std::vector<Route>& getRoutes() const {
        return routes;
    }

    const RegexList& getNoStoreTargets() const {
        return noStoreTargets;
    }

    const RegexList& getPrivateTargets() const {
        return privateTargets;
    }

    const RegexList& getPublicTargets() const {
        return publicTargets;
    }

    const RegexList& getLongTermTargets() const {
        return longTermTargets;
    }

    int getMaxAgeDefault() const {
        return maxAgeDefault;
    }

    int getMaxAgeLongTerm() const {
        return maxAgeLongTerm;
    }

    const boost::optional<std::string>& getCertFilename() const {
        return certFilename;
    }

    const boost::optional<std::string>& getCertKeyFilename() const {
        return certKeyFilename;
    }

    const boost::optional<std::string>& getCertKeyPassphrase() const {
        return certKeyPassphrase;
    }
};
