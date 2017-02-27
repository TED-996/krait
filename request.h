#pragma once
#include<string>
#include<map>
#include"http.h"


class Request {
private:
	HttpVerb verb;
	std::string url;
	int httpMajor;
	int httpMinor;
	std::map<std::string, std::string> headers;
	std::string body;

public:
	Request(HttpVerb verb, const std::string& url, int httpMajor, int httpMinor,
	        const std::map<std::string, std::string>& headers, const std::string& body);

	bool headerExists(std::string name);
	const std::string* getHeader(const std::string& name) const;

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
	bool isKeepAlive() const;
	int getKeepAliveTimeout() const;

	void setVerb(HttpVerb verb) {
		this->verb = verb;
	}
};


class RequestParser {
	int state;
	std::string workingBackBuffer;
	char workingStr[65536];
	unsigned int workingIdx;

	bool finished;
	unsigned long long bodyLeft;

	std::string workingHeaderName;
	std::string workingHeaderValue;

	std::string methodString;
	std::string url;
	std::string httpVersion;
	std::map<std::string, std::string> headers;
	std::string body;

	void saveHeader(std::string name, std::string value);

	bool consumeOne(char chr);

	int getBodyLength();

public:
	RequestParser();

	void consume(char* data, int dataLen);

	bool isFinished() {
		return finished;
	}

	Request getRequest();
};
