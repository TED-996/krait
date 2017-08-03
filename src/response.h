#pragma once
#include <string>
#include <unordered_map>
#include <boost/optional.hpp>
#include "iteratorResult.h"

class Response
{
	int httpMajor;
	int httpMinor;

	int statusCode;

	std::unordered_multimap<std::string, std::string> headers;

	IteratorResult bodyIterator;
	bool connClose;

	std::string statusLine;
	bool fromFullResponse;

	void parseFullResponse(const  std::string& response);
	boost::optional<std::string> getHeader(std::string name);

public:
	Response(int httpMajor, int httpMinor, int statusCode, const std::unordered_multimap<std::string, std::string>& headers,
	         const std::string& body, bool connClose);
	explicit Response(const std::string& fullResponse);
	Response(int httpMajor, int httpMinor, int statusCode, const std::unordered_multimap<std::string, std::string>& headers,
	         const IteratorResult& bodyIterator, bool connClose);
	Response(int statusCode, const std::string& body, bool connClose);
	Response(int statusCode, const IteratorResult& bodyIterator, bool connClose);

	void setHttpVersion(int httpMajor, int httpMinor) {
		this->httpMajor = httpMajor;
		this->httpMinor = httpMinor;
	}

	void getStatusCode(int statusCode) {
		this->statusCode = statusCode;
	}

	int getStatusCode() {
		return statusCode;
	}

	void setBody(const std::string& body, bool updateLength);

	void addHeader(std::string name, const std::string& value);
	void setHeader(std::string name, const std::string& value);
	void removeHeader(std::string name);

	void setConnClose(bool connClose);

	bool headerExists(std::string name);

	std::string getResponseHeaders();
	const std::string* getBodyNext();
};
