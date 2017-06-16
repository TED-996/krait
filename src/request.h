#pragma once
#include<string>
#include<map>
#include <boost/optional.hpp>
#include"http.h"


class Request
{
private:
	HttpVerb verb;
	RouteVerb routeVerb;
	std::string url;
	int httpMajor;
	int httpMinor;
	std::map<std::string, std::string> headers;
	std::string body;
	std::string queryString;

public:
	Request(HttpVerb verb, const std::string& url, const std::string& queryString, int httpMajor, int httpMinor,
	        const std::map<std::string, std::string>& headers, const std::string& body);

	bool headerExists(std::string name);
	const boost::optional<std::string> getHeader(const std::string& name) const;

	const HttpVerb getVerb() const {
		return verb;
	};

	const std::string& getUrl() const {
		return url;
	}

	const int getHttpMajor() const {
		return httpMajor;
	}

	const int getHttpMinor() const {
		return httpMinor;
	}

	const std::map<std::string, std::string>& getHeaders() const {
		return headers;
	}

	const std::string& getBody() const {
		return body;
	}

	const std::string getQueryString() const {
		return queryString;
	}

	RouteVerb getRouteVerb() const {
		return routeVerb;
	}

	void setRouteVerb(RouteVerb routeVerb) {
		this->routeVerb = routeVerb;
	}

	bool isKeepAlive() const;
	int getKeepAliveTimeout() const;
	bool isUpgrade();

	bool isUpgrade(std::string protocol);

	void setVerb(HttpVerb verb) {
		this->verb = verb;
	}
};
