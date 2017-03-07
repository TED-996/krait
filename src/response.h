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
	bool connClose;
	
	std::string statusLine;
	bool fromFullResponse;
	
	void parseFullResponse(std::string response);
	
public:
	Response(int httpMajor, int httpMinor, int statusCode, std::unordered_map<std::string, std::string> headers,
	         std::string body, bool connClose);
	Response(std::string fullResponse);

	void setHttpVersion(int httpMajor, int httpMinor) {
		this->httpMajor = httpMajor;
		this->httpMinor = httpMinor;
	}

	void setHttpCode(int statusCode) {
		this->statusCode = statusCode;
	}

	void setBody(std::string body, bool updateLength);

	void addHeader(std::string name, std::string value);
	void setHeader(std::string name, std::string value);
	void removeHeader(std::string name);
	
	void setConnClose(bool connClose);

	bool headerExists(std::string name);

	std::string getResponseData();
};
