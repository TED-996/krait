#include<boost/format.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>

#include "response.h"
#include "except.h"

#define DBG_DISABLE
#include "dbg.h"

namespace b = boost;


static std::unordered_map<int, std::string> statusReasons {
	{101, "Switching Protocols"},
	{200, "OK"},
	{304, "Not Modified"},
	{400, "Bad Request"},
	{401, "Unauthorized"},
	{403, "Forbidden"},
	{404, "Not Found"},
	{500, "Internal Server Error"}
};


Response::Response(int httpMajor, int httpMinor, int statusCode, std::unordered_multimap<std::string, std::string> headers,
                   std::string body, bool connClose)
	: bodyIterator(body){
	this->httpMajor = httpMajor;
	this->httpMinor = httpMinor;
	this->statusCode = statusCode;
	this->fromFullResponse = false;
	this->connClose = connClose;

	for (auto it : headers){
		this->headers.insert(make_pair(b::to_lower_copy(it.first), it.second));
	}

	setHeader("Content-Length", std::to_string(bodyIterator.getTotalLength()));
}

Response::Response(int httpMajor, int httpMinor, int statusCode, std::unordered_multimap<std::string, std::string> headers,
                   IteratorResult bodyIterator, bool connClose)
	: bodyIterator(bodyIterator){
	this->httpMajor = httpMajor;
	this->httpMinor = httpMinor;
	this->statusCode = statusCode;
	this->fromFullResponse = false;
	this->connClose = connClose;

	for (auto it : headers){
		this->headers.insert(make_pair(b::to_lower_copy(it.first), it.second));
	}

	setHeader("Content-Length", std::to_string(bodyIterator.getTotalLength()));
}

Response::Response(int statusCode, std::string body, bool connClose)
	: Response(1, 1, statusCode, std::unordered_multimap<std::string, std::string>(), body, connClose) {
}

Response::Response(int statusCode, IteratorResult body, bool connClose)
		: Response(1, 1, statusCode, std::unordered_multimap<std::string, std::string>(), body, connClose) {
}

Response::Response(std::string fullResponse)
	: bodyIterator(""){
	this->fromFullResponse = true;
	parseFullResponse(fullResponse);
}


void Response::parseFullResponse(std::string response){
	size_t statusEnd = response.find("\r\n");
	statusLine = response.substr(0, statusEnd);

	size_t headersEnd = response.find("\r\n\r\n");
	std::string newBody = response.substr(headersEnd + 4, std::string::npos);
	std::string headersStr = response.substr(statusEnd + 2, headersEnd - (statusEnd + 2));

	size_t idx = 0;
	while (idx != std::string::npos){
		size_t nextIdx = headersStr.find("\r\n", idx + 2);
		size_t colonIdx = headersStr.find(": ", idx);

		headers.insert(std::make_pair(boost::trim_copy(boost::to_lower_copy(headersStr.substr(idx, colonIdx - idx))),
		                              boost::trim_copy(headersStr.substr(colonIdx + 2, nextIdx - (colonIdx + 2)))));

		idx = nextIdx;
	}

	setBody(newBody, true);
}


void Response::setBody(std::string body, bool updateLength) {
	this->bodyIterator = IteratorResult(body);

	if (updateLength){
		setHeader("Content-Length", std::to_string(bodyIterator.getTotalLength()));
	}
}


void Response::addHeader(std::string name, std::string value) {
	boost::to_lower(name);
	headers.insert(make_pair(name, value));
}


void Response::setHeader(std::string name, std::string value) {
	boost::to_lower(name);
	removeHeader(name);
	addHeader(name, value);
}

void Response::removeHeader(std::string name) {
	boost::to_lower(name);
	auto its = headers.equal_range(name);

	if (its.first != headers.end()){
		headers.erase(its.first, its.second);
	}
}

bool Response::headerExists(std::string name) {
	boost::to_lower(name);
	auto headerIt = headers.find(name);

	return (headerIt != headers.end());
}

boost::optional<std::string> Response::getHeader(std::string name) {
	boost::to_lower(name);
	auto headerIt = headers.find(name);

	if (headerIt == headers.end()) {
		return b::none;
	}
	else{
		return headerIt->second;
	}
}

void Response::setConnClose(bool connClose) {
	this->connClose = connClose;
	boost::optional<std::string> header = getHeader("Connection");
	if (header == boost::none || header.get() == "close" || header.get() == "keep-alive") {
		if (connClose) {
			setHeader("Connection", "close");
		} else {
			removeHeader("Connection");
		}
	}
}

std::string getStatusReason(int statusCode);
std::string formatTitleCase(std::string str);

std::string Response::getResponseHeaders() {
	setConnClose(connClose);

	std::string statusLine;
	if (fromFullResponse){
		statusLine = this->statusLine;
	}
	else{
		statusLine = (boost::format("HTTP/%1%.%2% %3% %4%") % httpMajor % httpMinor % statusCode % getStatusReason(statusCode)).str();
	}
	std::vector<std::string> headerStrings;

	boost::format headerFormat = boost::format("%1%: %2%");
	for (auto header : headers) {
		headerStrings.push_back((boost::format(headerFormat) % formatTitleCase(header.first) % header.second).str());
	}
	headerStrings.push_back(std::string());

	std::string headersAll = b::algorithm::join(headerStrings, "\r\n");

	return statusLine + "\r\n" + headersAll + "\r\n";
}

const std::string* Response::getBodyNext() {
	const std::string* result = *bodyIterator;
	++bodyIterator;
	return result;
}


std::string getStatusReason(int statusCode) {
	auto it = statusReasons.find(statusCode);

	if (it == statusReasons.end()) {
		return std::string("Reason unknown.");
	}
	else {
		return it->second;
	}
}


std::string formatTitleCase(std::string str) {
	if (str.length() != 0){
		str[0] = (char) toupper(str[0]);
	}
	size_t idx = str.find('-');
	while (idx != std::string::npos){
		if (idx != str.length() - 1){
			str[idx + 1] = (char) toupper(str[idx + 1]);
		}
		idx = str.find('-', idx + 1);
	}
	return str;
}

