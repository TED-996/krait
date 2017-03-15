#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include "requestParser.h"
#include "fsm.h"
#include "dbg.h"

using namespace std;
using namespace boost;
using namespace boost::algorithm;


RequestParser::RequestParser() {
	state = 0;
	finished = false;
	memzero(workingStr);
	workingIdx = 0;
	bodyLeft = 0;
}


bool RequestParser::consumeOne(char chr) {
	if (chr == '\0') {
		BOOST_THROW_EXCEPTION(httpParseError() << stringInfo("Got null character!"));
	}

	FsmStart(int, state, char, chr, workingStr, sizeof(workingStr), workingIdx, workingBackBuffer, NULL, NULL)
	StatesBegin(0) { //Before any; skipping spaces before HTTP method
		SaveStart()
		TransIf(' ', stCurrent, true)
		TransElse(1, true)
	}
	StatesNext(1) { //Reading HTTP Method
		TransIf(' ', 2, true)
		TransElse(stCurrent, true)
		SaveThisIfSame()
	}
	StatesNext(2) { //Finished reading HTTP method; skipping spaces before URL
		SaveStoreOne(methodString)
		SaveStart()
		TransIf(' ', stCurrent, true)
		TransElse(3, true)
	}
	StatesNext(3) { //Reading URL
		TransIf(' ', 4, true)
		TransElse(stCurrent, true)
		SaveThisIfSame()
	}
	StatesNext(4) { //Finished reading URL; skipping before HTTP version
		SaveStoreOne(url)
		SaveStart()
		TransIf(' ', stCurrent, true)
		TransElse(5, true)
	}
	StatesNext(5) { //Reading HTTP version
		TransIf('\r', 6, true)
		TransElse(stCurrent, true)
		SaveThisIfSame()
	}
	StatesNext(6) { //After HTTP version
		SaveStore(httpVersion)
		TransIf('\n', 7, true)
		TransElseError()
	}
	StatesNext(7) { //Header start or CRLF; TODO: Some headers start with space!
		TransIf('\r', 20, true)
		TransElse(8, false)
	}
	StatesNext(8) { //Starting header name
		SaveStart()
		TransAlways(9, true)
	}
	StatesNext(9) //Consuming header name
		TransIf(':', 10, true)
		TransElse(stCurrent, true)
		SaveThisIfSame()
	StatesNext(10) { //After header name
		SaveStore(workingHeaderName)
		TransIf(' ', 11, true)
		TransElseError()
	}
	StatesNext(11) { //Starting header value
		SaveStart()
		TransAlways(12, true)
	}
	StatesNext(12) { //Consuming header value
		TransIf('\r', 13, true)
		TransElse(stCurrent, true)
		SaveThisIfSame()
	}
	StatesNext(13) { //inside CRLF before a header
		SaveStore(workingHeaderValue);
		saveHeader(workingHeaderName, workingHeaderValue);
		TransIf('\n', 7, true)
		TransElseError()
	}
	StatesNext(20) {//inside CRLF before body / end:
		TransIf('\n', 21, true)
		TransElseError()

		bodyLeft = getBodyLength();
		if (bodyLeft == 0) {
			finished = true;
		}
	}
	StatesNext(21) { //First character of body
		if (finished) {
			BOOST_THROW_EXCEPTION(httpParseError() << stringInfo("Request already finished, but received data!"));
		}
		SaveStart()
		TransAlways(22, true)

		bodyLeft--;
		if (bodyLeft == 0) {
			finished = true;
			SaveStore(body)
			TransAlways(25, true)
		}
	}
	StatesNext(22) { //Characters of body
		SaveThis()

		bodyLeft--;
		if (bodyLeft == 0) {
			finished = true;
			SaveStore(body)
			TransAlways(25, true)
		}
	}
	StatesNext(25) {
		BOOST_THROW_EXCEPTION(httpParseError() << stringInfo("Request already finished, but received data!"));

	}
	StatesEnd()
	FsmEnd()
}


int RequestParser::getBodyLength() {
	auto foundHeader = headers.find("content-length");
	if (foundHeader == headers.end()) {
		return 0;
	}
	else {
		size_t idx = 0;
		int length = stoi(foundHeader->second, &idx, 10);

		if (idx != foundHeader->second.size()) {
			BOOST_THROW_EXCEPTION(httpParseError() << stringInfo("Content-Length not int!"));
		}

		return length;
	}
}

void RequestParser::saveHeader(string name, string value) {
	to_lower(name);
	auto foundName = headers.find(name);
	if (foundName == headers.end()) {
		headers.insert(make_pair(name, value));
	}
	else {
		headers[name] = headers[name] + "," + value;
	}
}


void RequestParser::consume(char* data, int dataLen) {
	//DBG_FMT("Consuming string of length %1% (real len %2%)", dataLen, strlen(data));
	for (int i = 0; i < dataLen; i++) {
		//DBG_FMT("i = %1%; chr = %2%", i, int(data[i]));
		bool consume = consumeOne(data[i]);
		if (!consume) {
			i--;
		}
	}
}


bool parseHttpVersion(string httpVersion, int* httpMajor, int* httpMinor);

Request RequestParser::getRequest() {
	map<string, HttpVerb> stringVerbMapping = {
			{"GET", HttpVerb::GET},
			{"HEAD", HttpVerb::HEAD},
			{"POST", HttpVerb::POST},
			{"PUT", HttpVerb::PUT},
			{"DELETE", HttpVerb::DELETE},
			{"CONNECT", HttpVerb::CONNECT},
			{"OPTIONS", HttpVerb::OPTIONS},
			{"TRACE", HttpVerb::TRACE}
	};

	auto verbIt = stringVerbMapping.find(methodString);
	if (verbIt == stringVerbMapping.end()) {
		BOOST_THROW_EXCEPTION(httpParseError() << stringInfoFromFormat("HTTP method %1% not recognized.", methodString));
	}
	HttpVerb verb = verbIt->second;

	int httpMinor = 1;
	int httpMajor = 1;

	if (!parseHttpVersion(httpVersion, &httpMajor, &httpMinor)) {
		BOOST_THROW_EXCEPTION(httpParseError() << stringInfo("HTTP version not recognized."));
	}

	string queryString;
	size_t queryStringStart = url.find('?');
	if (queryStringStart != string::npos){
		queryString = url.substr(queryStringStart + 1);
		url.erase(queryStringStart);
	}

	return Request(verb, url, queryString, httpMajor, httpMinor, headers, body);
}

bool parseHttpVersion(string httpVersion, int* httpMajor, int* httpMinor) {
	DBG_FMT("Version: %1%", httpVersion);
	if (!boost::starts_with(httpVersion, "HTTP/")) {
		DBG_FMT("Version fail: %1% doesn't start with 'HTTP/'", httpVersion);
		return false;
	}

	string versionPart = httpVersion.substr(strlen("HTTP/"));
	DBG_FMT("Version part: %1%", versionPart);

	if (versionPart.size() != 3) {
		DBG_FMT("Version fail: %1%'s part %2% doesn't have 3 characters", httpVersion, versionPart);
		return false;
	}

	size_t idx = 0;
	*httpMajor = stoi(versionPart, &idx, 10);
	if (idx != 1 || versionPart[1] != '.') {
		DBG_FMT("Version fail: %1%'s part %2% doesn't start with a single digit or doesn't have the slash", httpVersion,
		        versionPart);
		return false;
	}
	*httpMinor = stoi(versionPart.substr(2), &idx, 10);
	if (idx != 1) {
		DBG_FMT("Version fail: %1%'s part %2% doesn't end with a single digit (idx = %3%)", httpVersion, versionPart, idx);
		return false;
	}

	return true;
}


