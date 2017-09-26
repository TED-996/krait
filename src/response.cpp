#include<boost/format.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string.hpp>
#include "response.h"
#include "except.h"

#define DBG_DISABLE
#include "dbg.h"
#include "dbgStopwatch.h"

namespace b = boost;


static std::unordered_map<int, std::string> statusReasons{
	{101, "Switching Protocols"},
	{200, "OK"},
	{304, "Not Modified"},
	{400, "Bad Request"},
	{401, "Unauthorized"},
	{403, "Forbidden"},
	{404, "Not Found"},
	{500, "Internal Server Error"}
};


Response::Response(int httpMajor, int httpMinor, int statusCode, const std::unordered_multimap<std::string, std::string>& headers,
                   const std::string& body, bool connClose)
	: bodyIterator(body) {
	this->httpMajor = httpMajor;
	this->httpMinor = httpMinor;
	this->statusCode = statusCode;
	this->fromFullResponse = false;
	this->connClose = connClose;

	for (auto it : headers) {
		this->headers.insert(make_pair(b::to_lower_copy(it.first), it.second));
	}

	setHeader("Content-Length", std::to_string(bodyIterator.getTotalLength()));
}

Response::Response(int httpMajor, int httpMinor, int statusCode, const std::unordered_multimap<std::string, std::string>& headers,
                   IteratorResult&& bodyIterator, bool connClose)
	: bodyIterator(std::move(bodyIterator)) {
	this->httpMajor = httpMajor;
	this->httpMinor = httpMinor;
	this->statusCode = statusCode;
	this->fromFullResponse = false;
	this->connClose = connClose;

	for (auto it : headers) {
		this->headers.insert(make_pair(b::to_lower_copy(it.first), it.second));
	}

	setHeader("Content-Length", std::to_string(bodyIterator.getTotalLength()));
}

Response::Response(int statusCode, const std::string& body, bool connClose)
	: bodyIterator(body) {
	this->httpMajor = 1;
	this->httpMinor = 1;
	this->statusCode = statusCode;
	this->fromFullResponse = false;
	this->connClose = connClose;

	setHeader("Content-Length", std::to_string(bodyIterator.getTotalLength()));
}

Response::Response(int statusCode, IteratorResult&& body, bool connClose)
	//: Response(1, 1, statusCode, std::unordered_multimap<std::string, std::string>(), body, connClose) {
	: bodyIterator(std::move(body)) {
	this->httpMajor = 1;
	this->httpMinor = 1;
	this->statusCode = statusCode;
	this->fromFullResponse = false;
	this->connClose = connClose;

	setHeader("Content-Length", std::to_string(bodyIterator.getTotalLength()));
}

Response::Response(const std::string& fullResponse)
	: bodyIterator("") {
	this->fromFullResponse = true;
	parseFullResponse(fullResponse);
}


void Response::parseFullResponse(const std::string& response) {
	size_t statusEnd = response.find("\r\n");
	statusLine = response.substr(0, statusEnd);

	size_t headersEnd = response.find("\r\n\r\n");
	std::string newBody = response.substr(headersEnd + 4, std::string::npos);
	std::string headersStr = response.substr(statusEnd + 2, headersEnd - (statusEnd + 2));

	size_t idx = 0;
	while (idx != std::string::npos) {
		size_t nextIdx = headersStr.find("\r\n", idx + 2);
		size_t colonIdx = headersStr.find(": ", idx);

		headers.insert(std::make_pair(b::trim_copy(b::to_lower_copy(headersStr.substr(idx, colonIdx - idx))),
		                              b::trim_copy(headersStr.substr(colonIdx + 2, nextIdx - (colonIdx + 2)))));

		idx = nextIdx;
	}

	setBody(newBody, true);
}


void Response::setBody(const std::string& body, bool updateLength) {
	this->bodyIterator = IteratorResult(body);

	if (updateLength) {
		setHeader("Content-Length", std::to_string(bodyIterator.getTotalLength()));
	}
}


void Response::addHeader(std::string name, const std::string& value) {
	b::to_lower(name);
	headers.insert(make_pair(name, value));
}


void Response::setHeader(std::string name, const std::string& value) {
	b::to_lower(name);
	removeHeader(name);
	addHeader(name, value);
}

void Response::removeHeader(std::string name) {
	b::to_lower(name);
	auto its = headers.equal_range(name);

	if (its.first != headers.end()) {
		headers.erase(its.first, its.second);
	}
}

bool Response::headerExists(std::string name) const {
	b::to_lower(name);
	auto headerIt = headers.find(name);

	return (headerIt != headers.end());
}

b::optional<std::string> Response::getHeader(std::string name) const {
	b::to_lower(name);
	auto headerIt = headers.find(name);

	if (headerIt == headers.end()) {
		return b::none;
	}
	else {
		return headerIt->second;
	}
}

void Response::setConnClose(bool connClose) {
	this->connClose = connClose;
	b::optional<std::string> header = getHeader("Connection");
	if (header == b::none || header.get() == "close" || header.get() == "keep-alive") {
		if (connClose) {
			setHeader("Connection", "close");
		}
		else {
			removeHeader("Connection");
		}
	}
}

bool Response::isSpecialConnHeader() const {
	b::optional<std::string> header = getHeader("Connection");
	if (header == b::none || header.get() == "close") {
		return false;
	}
	return true;
}

std::string getStatusReason(int statusCode);
std::string formatTitleCase(std::string str);

std::string Response::getResponseHeaders() const {
	std::string statusLine;
	if (fromFullResponse) {
		statusLine = this->statusLine;
	}
	else {
		statusLine = (b::format("HTTP/%1%.%2% %3% %4%") % httpMajor % httpMinor % statusCode % getStatusReason(statusCode)).str();
	}
	std::vector<std::string> headerStrings;

	b::format headerFormat = b::format("%1%: %2%");

	bool specialConnHeader = isSpecialConnHeader();
	if (connClose && !specialConnHeader) {
		headerStrings.push_back("Connection: close");
	}
	for (auto header : headers) {
		//We skip "connection" (already added) for connection:close; otherwise we keep it
		if (specialConnHeader || b::to_lower_copy(header.first) != "connection") {
			headerStrings.push_back((b::format(headerFormat) % formatTitleCase(header.first) % header.second).str());
		}
	}

	headerStrings.push_back(std::string());

	std::string headersAll = b::algorithm::join(headerStrings, "\r\n");

	return statusLine + "\r\n" + headersAll + "\r\n";
}

const std::string* Response::getBodyNext() {
	DbgStopwatch stopwatch("Response::getBodyNext");
	(void)stopwatch;

	static const size_t strCapacity = 65536;
	static std::string concatStr(strCapacity, '\0');
	concatStr.clear();  // Usually this does NOT resize the string. We need the capacity!

	if (concatStr.capacity() < strCapacity) {
		DBG("std::string implementation doesn't keep the capacity over a clear(), disabling optimization.");
		const std::string* result = *bodyIterator;
		++bodyIterator;
		return result;
	}

	bool isConcat = false;
	const std::string* firstString = nullptr;

	/* Loop invariant:
	 * Either:
	 *		1. firstString == nullptr, in which case no string has been read yet
	 *		2. firstString != nullptr and !isConcat, in which case we read ONLY the first string but
	 *			we're not sure whether to concat it or send it
	 *		3. firstString != nullptr and isConcat, in which case we've read AT LEAST 2 strings and
	 *			ALL strings read so far are in concatStr
	 */

	while (true) {
		const std::string* thisString = *bodyIterator;
		if (thisString == nullptr) {
			// Case 3:
			if (isConcat) {
				return &concatStr;
			}
			// Case 2:
			if (firstString != nullptr) {
				return firstString;
			}
			// Case 1:
			if (firstString == nullptr) {
				return nullptr;
			}
			// Yeah, there's some redunancy, but Clang will definitely pick up on that, right?
		}

		// Case 1: If it's the first and already doesn't fit, send it.
		if (firstString == nullptr && thisString->size() > strCapacity) {
			++bodyIterator;
			return thisString;
		}

		// Case 3:
		if (isConcat) {
			// If we've already concatted and it does fit, keep it
			if (thisString->size() <= concatStr.capacity() - concatStr.size()) {
				concatStr.append(*thisString);

				//Increment since we're keeping it.
				++bodyIterator;
			}
			// If we've already concatted, but it doesn't fit, scrap it
			else {
				//Don't increment since we're scrapping it.
				return &concatStr;
			}
		}
		//Case 1: It's the first AND it fits (see first if)
		else if (firstString == nullptr) {
			firstString = thisString;
			// Increment since we're keeping it.
			++bodyIterator;
		}
		// Case 2: It's the second string, after the first one fit.
		else {
			// If both strings fit, concat them.
			if (firstString->size() + thisString->size() <= strCapacity) {
				concatStr.append(*firstString);
				concatStr.append(*thisString);
				isConcat = true;

				// Increment since we're keeping the second one.
				++bodyIterator;
			}
			// If only the first fits, send the first one and scrap the second.
			else {
				// Don't increment since we're scrapping the second one.
				return firstString;
			}
		}
	}
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
	if (str.length() != 0) {
		str[0] = (char)toupper(str[0]);
	}
	size_t idx = str.find('-');
	while (idx != std::string::npos) {
		if (idx != str.length() - 1) {
			str[idx + 1] = (char)toupper(str[idx + 1]);
		}
		idx = str.find('-', idx + 1);
	}
	return str;
}
