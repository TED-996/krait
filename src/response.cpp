#include<boost/format.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>

#include "response.h"
#include "except.h"

#define DBG_DISABLE
#include "dbg.h"

using namespace std;
using namespace boost;


static unordered_map<int, string> statusReasons {
	{200, "OK"},
	{304, "Not Modified"},
	{400, "Bad Request"},
	{401, "Unauthorized"},
	{403, "Forbidden"},
	{404, "Not Found"},
	{500, "Internal Server Error"}
};


Response::Response(int httpMajor, int httpMinor, int statusCode, unordered_multimap<string, string> headers,
                   string body, bool connClose)
	: bodyIterator(body){
	this->httpMajor = httpMajor;
	this->httpMinor = httpMinor;
	this->statusCode = statusCode;
	this->fromFullResponse = false;
	this->connClose = connClose;

	for (auto it : headers){
		this->headers.insert(make_pair(to_lower_copy(it.first), it.second));
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
		this->headers.insert(make_pair(to_lower_copy(it.first), it.second));
	}

	setHeader("Content-Length", std::to_string(bodyIterator.getTotalLength()));
}

Response::Response(int statusCode, std::string body, bool connClose)
	: Response(1, 1, statusCode, unordered_multimap<string, string>(), body, connClose) {
}

Response::Response(int statusCode, IteratorResult body, bool connClose)
		: Response(1, 1, statusCode, unordered_multimap<string, string>(), body, connClose) {
}

Response::Response(string fullResponse)
	: bodyIterator(""){
	this->fromFullResponse = true;
	parseFullResponse(fullResponse);
}


void Response::parseFullResponse(string response){
	size_t statusEnd = response.find("\r\n");
	statusLine = response.substr(0, statusEnd);

	size_t headersEnd = response.find("\r\n\r\n");
	string newBody = response.substr(headersEnd + 4, string::npos);
	string headersStr = response.substr(statusEnd + 2, headersEnd - (statusEnd + 2));

	size_t idx = 0;
	while (idx != string::npos){
		size_t nextIdx = headersStr.find("\r\n", idx + 2);
		size_t colonIdx = headersStr.find(": ", idx);

		headers.insert(make_pair(trim_copy(to_lower_copy(headersStr.substr(idx, colonIdx - idx))),
		                         trim_copy(headersStr.substr(colonIdx + 2, nextIdx - (colonIdx + 2)))));

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


void Response::addHeader(string name, string value) {
	to_lower(name);
	headers.insert(make_pair(name, value));
}


void Response::setHeader(string name, string value) {
	to_lower(name);
	removeHeader(name);
	addHeader(name, value);
}

void Response::removeHeader(string name) {
	to_lower(name);
	auto its = headers.equal_range(name);

	if (its.first != headers.end()){
		headers.erase(its.first, its.second);
	}
}

void Response::setConnClose(bool connClose) {
	this->connClose = connClose;
	if (connClose){
		setHeader("Connection", "close");
	}
	else{
		removeHeader("Connection");
	}
}
string getStatusReason(int statusCode);

string formatTitleCase(string str);


string Response::getResponseHeaders() {
	setConnClose(connClose);

	string statusLine;
	if (fromFullResponse){
		statusLine = this->statusLine;
	}
	else{
		statusLine = (format("HTTP/%1%.%2% %3% %4%") % httpMajor % httpMinor % statusCode % getStatusReason(statusCode)).str();
	}
	vector<string> headerStrings;

	format headerFormat = format("%1%: %2%");
	for (auto header : headers) {
		headerStrings.push_back((format(headerFormat) % formatTitleCase(header.first) % header.second).str());
	}
	headerStrings.push_back(string());

	string headersAll = algorithm::join(headerStrings, "\r\n");

	return statusLine + "\r\n" + headersAll + "\r\n";
}

bool Response::headerExists(string name) {
	to_lower(name);
	auto headerIt = headers.find(name);

	return (headerIt != headers.end());
}

const std::string* Response::getBodyNext() {
	const string* result = *bodyIterator;
	++bodyIterator;
	return result;
}


string getStatusReason(int statusCode) {
	auto it = statusReasons.find(statusCode);

	if (it == statusReasons.end()) {
		return string("Reason unknown.");
	}
	else {
		return it->second;
	}
}


string formatTitleCase(string str) {
	if (str.length() != 0){
		str[0] = (char) toupper(str[0]);
	}
	size_t idx = str.find('-');
	while (idx != string::npos){
		if (idx != str.length() - 1){
			str[idx + 1] = (char) toupper(str[idx + 1]);
		}
		idx = str.find('-', idx + 1);
	}
	return str;
}

