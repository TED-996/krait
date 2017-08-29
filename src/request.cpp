#include<string>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/lexical_cast.hpp>
#include"request.h"
#include"logger.h"

#define DBG_DISABLE
#include"dbg.h"

namespace b = boost;
namespace ba = boost::algorithm;


Request::Request(HttpVerb verb, const std::string& url, const std::string& queryString, int httpMajor, int httpMinor,
                 const std::map<std::string, std::string>& headers, const std::string& body) {
	this->verb = verb;
	this->url = url; 
	this->httpMajor = httpMajor;
	this->httpMinor = httpMinor;
	this->headers = std::map<std::string, std::string>();
	for (auto p : headers) {
		this->headers[ba::to_lower_copy(p.first)] = p.second;
	}

	this->body = std::string(body);
	this->queryString = queryString;

	this->routeVerb = toRouteVerb(verb);
}


Request::Request(HttpVerb verb, std::string&& url, std::string&& queryString, int httpMajor, int httpMinor,
                 const std::map<std::string, std::string>& headers, std::string&& body) : 
	url(std::move(url)), body(std::move(body)), queryString(std::move(queryString)){
	this->verb = verb;
	this->httpMajor = httpMajor;
	this->httpMinor = httpMinor;
	
	for (auto p : headers) {
		this->headers[ba::to_lower_copy(p.first)] = p.second;
	}
	this->routeVerb = toRouteVerb(verb);
}

Request::Request(Request&& other) noexcept
	: url(std::move(other.url)), headers(other.headers), body(std::move(other.body)), queryString(std::move(other.queryString)) {
	this->verb = other.verb;
	this->httpMajor = other.httpMajor;
	this->httpMinor = other.httpMinor;
	this->routeVerb = other.routeVerb;
}

bool Request::headerExists(std::string name) const {
	const auto iterFound = headers.find(ba::to_lower_copy(name));
	return (iterFound != headers.end());
}

boost::optional<std::string> Request::getHeader(const std::string& name) const {
	const auto iterFound = headers.find(ba::to_lower_copy(name));
	if (iterFound == headers.end()) {
		return boost::none;
	}
	return iterFound->second;
}


bool Request::isKeepAlive() const {
	auto it = headers.find("connection");

	if (httpMajor < 1 || (httpMajor == 1 && httpMinor < 1)) {
		if (it == headers.end()) {
			return false;
		}
		return ba::to_lower_copy(it->second) == "keep-alive";
	}
	else {
		if (it == headers.end()) {
			return true;
		}
		return ba::to_lower_copy(it->second) != "close";
	}
}

int Request::getKeepAliveTimeout() const {
	if (!this->isKeepAlive()) {
		return 0;
	}
	auto it = headers.find("keep-alive");
	if (it == headers.end()) {
		return INT_MAX; //will be set with a minimum.
	}

	std::string value = it->second;
	if (!boost::starts_with(value, "timeout=")) {
		return 0; //other unsupported timeout types, ignore.
	}

	std::string secondsStr = value.substr(std::string("timeout=").length());
	try {
		return boost::lexical_cast<int>(secondsStr);
	}
	catch (const b::bad_lexical_cast&) {
		Loggers::logErr("Received non-integer as timeout seconds\n");
		return 0; //should we throw?
	}
}

bool Request::isUpgrade() {
	return getHeader("upgrade") != boost::none;
}


bool Request::isUpgrade(std::string protocol) {
	boost::optional<std::string> upgradeHeaderOpt = getHeader("upgrade");
	if (upgradeHeaderOpt == boost::none) {
		return false;
	}
	std::string& upgradeHeader = upgradeHeaderOpt.get();
	size_t valueStart = 0;
	size_t commaIdx = upgradeHeader.find(',', 0);
	while (commaIdx != std::string::npos) {
		if (upgradeHeader.compare(valueStart, commaIdx - valueStart, protocol) == 0) {
			return true;
		}
		valueStart = commaIdx + 1;
		commaIdx = upgradeHeader.find(',', valueStart);
	};
	if (upgradeHeader.compare(valueStart, std::string::npos, protocol) == 0) {
		return true;
	}
	else {
		return false;
	}
}
