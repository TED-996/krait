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
	Response(int httpMajor, int httpMinor, int httpCode, std::unordered_map<std::string, std::string> headers, std::string body);
	
	void setHttpVersion(int httpMajor, int httpMinor) {
		this->httpMajor = httpMajor;
		this->httpMinor = httpMinor;
	}
	
	void setHttpCode(int statusCode){
		this->statusCode = statusCode;
	}
	
	void setBody(std::string body){
		this->body = body;
	}
	
	void addHeader(std::string name, std::string value);
	
	std::string getResponseData();
};