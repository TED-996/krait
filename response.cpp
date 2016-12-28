#include<boost/format.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/assign.hpp>
#include "response.h"
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


Response::Response(int httpMajor, int httpMinor, int statusCode, unordered_map<string, string> headers, string body) {
	this->httpMajor = httpMajor;
	this->httpMinor = httpMinor;
	this->statusCode = statusCode;
	this->headers = headers;

	setBody(body);
}

void Response::setBody(std::string body) {
	this->body = body;

	if (body.length() == 0) {
		removeHeader("Content-Length");
	}
	else {
		setHeader("Content-Length", std::to_string(body.length()));
	}
}


void Response::addHeader(string name, string value) {
	auto headerIt = headers.find(name);

	if (headerIt == headers.end()) {
		headers.insert(make_pair(name, value));
	}
	else {
		headers[name] = headers[name] + "," + value;
	}
}


void Response::setHeader(string name, string value) {
	headers[name] = value;
}


void Response::removeHeader(string name) {
	auto headerIt = headers.find(name);

	if (headerIt != headers.end()) {
		headers.erase(headerIt);
	}
}


string getStatusReason(int statusCode);

string Response::getResponseData() {
	string statusLine = (format("HTTP/%1%.%2% %3% %4%") % httpMajor % httpMinor % statusCode % getStatusReason(
	                         statusCode)).str();
	vector<string> headerStrings;

	format headerFormat = format("%1%: %2%");
	for (auto header : headers) {
		headerStrings.push_back((format(headerFormat) % header.first % header.second).str());
	}
	headerStrings.push_back(string());

	string headersAll = algorithm::join(headerStrings, "\r\n");

	string result = statusLine + "\r\n" + headersAll + "\r\n" + body;
	DBG_FMT("HTTP response:\n%1%", result);
	
	return result;
}


bool Response::headerExists(string name){
	auto headerIt = headers.find(name);

	if (headerIt == headers.end()) {
		return false;
	}
	else {
		return true;
	}
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

