#include"request.h"
#include"fsm.h"
#include<string>
#include<utility>
#include <boost/algorithm/string/predicate.hpp>

RequestParser::RequestParser() {
	state = 1;
	finished = false;
	memzero(workingStr);
	workingIdx = 0;
	bodyLeft = 0;
}


bool RequestParser::consumeOne ( char chr ) {
	FsmStart(int, state, char, chr, workingStr, sizeof(workingStr), workingIdx, workingBackBuffer)
		StatesBegin(0) //Before any; skipping spaces before HTTP method
			SaveStart()
			TransIf(' ', stCurrent, true)
			TransElse(1, true)
		StatesNext(1) //Reading HTTP Method
			TransIf(' ', 2, true)
			TransElse(stCurrent, true)
			SaveThisIfSame()
		StatesNext(2) //Finished reading HTTP method; skipping spaces before URL
			SaveStoreOne(methodString)
			SaveStart()
			TransIf(' ', stCurrent, true)
			TransElse(3, true)
		StatesNext(3) //Reading URL
			TransIf(' ', 4, true)
			TransElse(stCurrent, true)
			SaveThisIfSame()
		StatesNext(4) //Finished reading URL; skipping before HTTP version
			SaveStoreOne(url)
			SaveStart()
			TransIf(' ', stCurrent, true)
			TransElse(5, true)
		StatesNext(5) //Reading HTTP version
			TransIf('\r', 6, true)
			TransElse(stCurrent, true)
			SaveThisIfSame()
		StatesNext(6) //After HTTP version
			SaveStore(httpVersion)
			TransIf('\n', 7, true)
			TransElseError()
		StatesNext(7) //Header start or CRLF; TODO: Some headers start with space!
			TransIf('\r', 20, true)
			TransElse(8, false)
		StatesNext(8) //Starting header name
			SaveStart()
			TransAlways(9, true)
		StatesNext(9) //Consuming header name
			TransIf(':', 10, true)
			TransElse(stCurrent, true)
			SaveThisIfSame()
		StatesNext(10) //After header name
			SaveStore(workingHeaderName)
			TransIf(' ', 11, true)
			TransElseError()
		StatesNext(11) //Starting header value
			SaveStart()
			TransAlways(12, true)
		StatesNext(12) //Consuming header value
			TransIf('\r', 13, true)
			TransElse(stCurrent, true)
			SaveThisIfSame()
		StatesNext(13) //inside CRLF before a header
			SaveStore(workingHeaderValue);
			saveHeader(workingHeaderName, workingHeaderValue);
			TransIf('\n', 7, true)
			TransElseError()
		StatesNext(20) //inside CRLF before body / end:
			TransIf('\n', 21, true)
			TransElseError()
			
			bodyLeft = getBodyLength();
			if (bodyLeft == 0){
				finished = true;
			}
		StatesNext(21) //First character of body
			if (finished){
				throw parseError() << stringInfo("Request already finished, but received data!");
			}
			SaveStart()
			TransAlways(22, true)
			
			bodyLeft--;
			if (bodyLeft == 0){
				finished = true;
				SaveStore(body)
				TransAlways(25, true)
			}
		StatesNext(22) //Characters of body
			SaveThis()
			
			bodyLeft--;
			if (bodyLeft == 0){
				finished = true;
				SaveStore(body)
				TransAlways(25, true)
			}
		StatesNext(25)
			throw parseError() << stringInfo("Request already finished, but received data!");
		StatesEnd()
	FsmEnd(state, workingIdx)
}


int RequestParser::getBodyLength() {
	auto foundHeader = headers.find(std::string("Content-Length"));
	if (foundHeader == headers.end()){
		return 0;
	}
	else{
		size_t idx = 0;
		int length = std::stoi(foundHeader->second, &idx, 10);
		
		if (idx != foundHeader->second.size()){
			throw parseError() << stringInfo("Content-Length not int!");
		}
		
		return length;
	}
	
	throw notImplementedError() << stringInfo("RequestParser::getBodyLength()"); //TODO
}

void RequestParser::saveHeader ( std::string name, std::string value ) {
	auto foundName = headers.find(name);
	if (foundName == headers.end()){
		headers.insert(make_pair(name, value));
	}
	else{
		headers[name] = headers[name] + "," + value;
	}
}



void RequestParser::consume ( char* data, int dataLen ) {
	for (int i = 0; i < dataLen; i++){
		bool consume = consumeOne(data[dataLen]);
		if (!consume){
			i--;
		}
	}
}


bool parseHttpVersion(std::string httpVersion, int* httpMajor, int* httpMinor);

Request RequestParser::getRequest() {
	std::map<std::string, HttpVerb> stringVerbMapping = {
		{std::string("GET"), HttpVerb::GET},
		{std::string("HEAD"), HttpVerb::HEAD},
		{std::string("POST"), HttpVerb::POST},
		{std::string("PUT"), HttpVerb::PUT},
		{std::string("DELETE"), HttpVerb::DELETE},
		{std::string("CONNECT"), HttpVerb::CONNECT},
		{std::string("OPTIONS"), HttpVerb::OPTIONS},
		{std::string("TRACE"), HttpVerb::TRACE}
	};
	
	auto verbIt = stringVerbMapping.find(methodString);
	if (verbIt == stringVerbMapping.end()){
		throw parseError() << stringInfo("HTTP method not recognized.");
	}
	HttpVerb verb = verbIt->second;
	
	int httpMinor = 1;
	int httpMajor = 1;
	
	if (!parseHttpVersion(httpVersion, &httpMajor, &httpMinor)){
		throw parseError() << stringInfo("HTTP version not recognized.");
	}
	
	
	throw notImplementedError() << stringInfo("RequestParser::getRequest()"); //TODO
}

bool parseHttpVersion(std::string httpVersion, int* httpMajor, int* httpMinor){
	if (!boost::starts_with(httpVersion, "HTTP/")){
		return false;
	}
	
	std::string versionPart = httpVersion.substr(strlen("HTTP/"));
	if (versionPart.size() != 3){
		return false;
	}
	
	size_t idx = 0;
	*httpMajor = std::stoi(versionPart, &idx, 10);
	if (idx != 1 || versionPart[idx] != '/'){
		return false;
	}
	idx = 2;
	*httpMinor = std::stoi(versionPart, &idx, 10);
	if (idx != 3){
		return false;
	}
	
	return true;
}
