#include"request.h"
#include"fsm.h"

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
			headers[workingHeaderName] = workingHeaderValue;
			TransIf('\n', 7, true)
			TransElseError()
		StatesNext(20) //inside CRLF before body / end:
			TransIf('\n', 21, true)
			TransElseError()
			
			if (!hasBody()){
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


bool RequestParser::hasBody() {
	throw notImplementedError() << stringInfo("RequestParser::hasBody()"); //TODO
}


void RequestParser::consume ( char* data, int dataLen ) {
	throw notImplementedError() << stringInfo("RequestParser::consume()"); //TODO
}


Request RequestParser::getRequest() {
	throw notImplementedError() << stringInfo("RequestParser::getRequest()"); //TODO
}


