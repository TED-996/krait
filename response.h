#pragma once
#include <string>
#include <unordered_map>
#include <utility>

class Response {
	int httpMajor;
	int httpMinor;

	int statusCode;

	std::unordered_map<std::string, std::string> headers;

	std::string body;

public:
	Response(int httpMajor, int httpMinor, int statusCode, std::unordered_map<std::string, std::string> headers,
	         std::string body);

	void setHttpVersion(int httpMajor, int httpMinor) {
		this->httpMajor = httpMajor;
		this->httpMinor = httpMinor;
	}

	void setHttpCode(int statusCode) {
		this->statusCode = statusCode;
	}

	void setBody(std::string body);

	void addHeader(std::string name, std::string value);
	void setHeader(std::string name, std::string value);
	void removeHeader(std::string name);

	bool headerExists(std::string name);

	std::string getResponseData();
};
