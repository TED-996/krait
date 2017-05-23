#include<string>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/lexical_cast.hpp>
#include"request.h"
#include"logger.h"

#define DBG_DISABLE
#include"dbg.h"

using namespace std;
using namespace boost::algorithm;
using namespace boost;


Request::Request(HttpVerb verb, const string& url, const string& queryString, int httpMajor, int httpMinor, const map<string, string>& headers,
                 const string& body) {
	this->verb = verb;
	this->url = url;
	this->httpMajor = httpMajor;
	this->httpMinor = httpMinor;
	this->headers = map<string, string>();
	for (auto p : headers){
		this->headers[to_lower_copy(p.first)] = p.second;
	}

	this->body = string(body);
	this->queryString = queryString;

	this->routeVerb = toRouteVerb(verb);
}

bool Request::headerExists(std::string name){
	const auto iterFound = headers.find(to_lower_copy(name));
	return (iterFound != headers.end());
}

const boost::optional<std::string> Request::getHeader(const string& name) const {
	const auto iterFound = headers.find(to_lower_copy(name));
	if (iterFound == headers.end()) {
		return boost::none;
	}
	return iterFound->second;
}


bool Request::isKeepAlive() const {
	auto it = headers.find("connection");
	
	if (httpMajor < 1 || (httpMajor == 1 && httpMinor < 1)){
		if (it == headers.end()){
			return false;
		}
		return boost::algorithm::to_lower_copy(it->second) == "keep-alive";
	}
	else{
		if (it == headers.end()){
			return true;
		}
		return boost::algorithm::to_lower_copy(it->second) != "close";
	}
}

int Request::getKeepAliveTimeout() const {
	if (!this->isKeepAlive()){
		return 0;
	}
	auto it = headers.find("keep-alive");
	if (it == headers.end()){
		return INT_MAX; //will be set with a minimum.
	}

	string value = it->second;
	if(!boost::starts_with(value, "timeout=")){
		return 0; //other unsupported timeout types, ignore.
	}

	string secondsStr = value.substr(string("timeout=").length());
	try{
		return boost::lexical_cast<int>(secondsStr);
	}
	catch(const bad_lexical_cast&){
		Loggers::logErr("Received non-integer as timeout seconds\n");
		return 0; //should we throw?
	}
}

bool Request::isUpgrade() {
	return getHeader("upgrade") != boost::none;
}


bool Request::isUpgrade(string protocol) {
	boost::optional<string> upgradeHeaderOpt = getHeader("upgrade");
	if (upgradeHeaderOpt == boost::none){
		return false;
	}
	string& upgradeHeader = upgradeHeaderOpt.get();
	size_t valueStart = 0;
	size_t commaIdx = upgradeHeader.find(',', 0);
	while (commaIdx != string::npos){
		if (upgradeHeader.compare(valueStart, commaIdx - valueStart, protocol) == 0){
			return true;
		}
		valueStart = commaIdx + 1;
		commaIdx = upgradeHeader.find(',', valueStart);
	};
	if (upgradeHeader.compare(valueStart, string::npos, protocol) == 0){
		return true;
	}
	else{
		return false;
	}
}
