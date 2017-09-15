#pragma once
#include <string>
#include <boost/optional.hpp>

class ArgvConfig
{
	std::string siteRoot;
	boost::optional<u_int16_t> httpPort;
	boost::optional<u_int16_t> httpsPort;
public:

	ArgvConfig(std::string siteRoot, boost::optional<u_int16_t> httpPort, boost::optional<u_int16_t> httpsPort)
		:
		siteRoot(std::move(siteRoot)),
		httpPort(httpPort),
		httpsPort(httpsPort) {
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
};
