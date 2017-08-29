#include "v2HttpParser.h"


V2HttpFsm V2HttpParser::fsm;
std::map<std::string, HttpVerb> V2HttpParser::methodStringMapping {
	{ "GET", HttpVerb::GET },
	{ "HEAD", HttpVerb::HEAD },
	{ "POST", HttpVerb::POST },
	{ "PUT", HttpVerb::PUT },
	{ "DELETE", HttpVerb::DELETE },
	{ "CONNECT", HttpVerb::CONNECT },
	{ "OPTIONS", HttpVerb::OPTIONS },
	{ "TRACE", HttpVerb::TRACE }
};


V2HttpFsm::V2HttpFsm() : FsmV2(20, 0), parser(nullptr) {
}

void V2HttpFsm::init() {
	typedef SimpleFsmTransition Simple;
	typedef ActionFsmTransition Action;
	typedef SavepointSetFsmTransition SavepointSet;
	typedef SavepointRevertFsmTransition SavepointRevert;
	typedef PushFsmTransition Push;
	typedef DiscardFsmTransition Discard;
	typedef AlwaysFsmTransition Always;
	typedef AndConditionFsmTransition Condition;
	typedef SkipFsmTransition Skip;
	typedef WhitespaceFsmTransition Whitespace;
	typedef FinalFsmTransition Final;
	typedef OrFinalFsmTransition OrFinal;
	typedef ErrorFsmTransition Error;

	enum {
		inMethod = 0,
		afterMethod,
		inUrl,
		afterUrl,
		inVersion,
		afterVersion,
		newLine,
		newLineInCrlf,
		inHeaderKey,
		afterHeaderKey,
		inHeaderValue,
		afterHeaderValue,
		inHeaderLws,
		inHeaderLwsInCrlf,
		inHeaderLwsAfterCrlf,
		beforeHeaderExtension,
		inHeaderExtension,
		inBody,
		errorState
	};

	//TODO: apparently, the messages may (if buggy client) start with CRLFs...
	add(inMethod, new HttpSetMethodTransition(&parser, new Skip(new Simple(' ', afterMethod))));
	add(inMethod, new Always(inMethod));

	add(afterMethod, new Always(inUrl));

	add(inUrl, new HttpSetUrlTransition(&parser, new Skip(new Simple(' ', afterUrl))));
	add(inUrl, new Always(inUrl));

	add(afterUrl, new Always(inVersion));

	add(inVersion, new HttpSetMethodTransition(&parser, new Skip(new Simple('\r', afterVersion))));
	add(inVersion, new Always(inVersion));

	add(afterVersion, new Skip(new Simple('\n', newLine)));
	add(afterVersion, new HttpErrorTransition(400, "Incomplete CR-LF after HTTP version.", &parser, new Always(errorState)));

	add(newLine, new Simple('\r', newLineInCrlf));

	// If a header line starts with a space or tab, it's a continuation line for the previous header's value
	add(newLine, new Simple(' ', beforeHeaderExtension));
	add(newLine, new Simple('\t', beforeHeaderExtension));

	add(newLine, new Discard(new Always(inHeaderKey)));

	add(newLineInCrlf, new HttpOnBodyStartTransition(&parser, new Discard(new Skip(new Simple('\n', inBody)))));
	add(newLineInCrlf, new HttpErrorTransition(400, "Incomplete CR-LF after CR-LF.", &parser, new Always(errorState)));

	add(inHeaderKey, new Push(new Skip(new Simple('=', afterHeaderKey))));
	add(inHeaderKey, new Always(inHeaderKey));

	add(afterHeaderKey, new Simple('\r', inHeaderLwsInCrlf));
	add(inHeaderLwsInCrlf, new HttpAddHeaderTransition(&parser, new Push(new Discard(new Skip(new Simple('\n', newLine))))));
	add(inHeaderLwsInCrlf, new HttpErrorTransition(400, "Incomplete CR-LF in LWS before header value.", &parser, new Always(errorState)));

	add(afterHeaderKey, new Simple(' ', afterHeaderKey));
	add(afterHeaderKey, new Simple('\t', afterHeaderKey));
	add(afterHeaderKey, new Discard(new Always(inHeaderValue)));

	add(inHeaderValue, new HttpAddHeaderTransition(&parser, new Push(new Simple('\r', afterHeaderValue))));
	add(inHeaderValue, new Always(inHeaderValue));

	//TODO: tolerate CRs without LFs? (assume part of header)

	add(afterHeaderValue, new Simple('\n', newLine));

	add(beforeHeaderExtension, new Simple(' ', beforeHeaderExtension));
	add(beforeHeaderExtension, new Simple('\t', beforeHeaderExtension));
	add(beforeHeaderExtension, new Simple('\r', inHeaderLwsInCrlf));
	add(beforeHeaderExtension, new Discard(new Always(inHeaderExtension)));

	add(inHeaderExtension, new HttpExtendHeaderTransition(&parser, new Simple('\r', afterHeaderValue)));
	add(inHeaderExtension, new Always(inHeaderExtension));

	add(inBody, new HttpSetBodyTransition(&parser, new Final(inBody)));
	add(inBody, new Always(inBody));
	add(errorState, new Always(errorState));

	addFinalActionToMany([=](FsmV2& fsm){
		BOOST_THROW_EXCEPTION(httpParseError() << stringInfo("Incomplete request. Insufficient information.")); //TODO: get more information
	}, {
			inMethod,
			afterMethod,
			inUrl,
			afterUrl,
			inVersion,
			afterVersion,
			newLine,
			newLineInCrlf,
			inHeaderKey,
			afterHeaderKey,
			inHeaderValue,
			afterHeaderValue,
			inHeaderLws,
			inHeaderLwsInCrlf,
			inHeaderLwsAfterCrlf,
			beforeHeaderExtension,
			inHeaderExtension
		});
	addFinalAction(errorState, [=](FsmV2 fsm) {
		BOOST_THROW_EXCEPTION(httpParseError() << stringInfo("Malformed HTTP request. Insufficient information."));
	});
}


V2HttpParser::V2HttpParser(int maxHeaderLength, int maxBodyLength) {
	this->maxHeaderLength = maxHeaderLength;
	this->maxBodyLegth = maxBodyLength;
	
	reset();
}

void V2HttpParser::reset() {
	method.clear();
	url.clear();
	version.clear();
	headers.clear();
	body.clear();

	lastHeaderKey.clear();
	
	bodyBytesLeft = 0;
	headerBytesLeft = maxHeaderLength;
	state = ParserState::InMethod;

	fsm.setParser(this);
}


void V2HttpParser::setMethod(std::string&& method) {
	if (method.length() >= 24) {
		onError(501, "Method too large");
	}
	else {
		this->method = std::move(method);
		this->state = ParserState::InUrl;
	}
}

void V2HttpParser::setUrl(std::string&& url) {
	if (url.length() >= 8192) {
		onError(414, "URI Too Long");
	}
	else {
		this->url = std::move(url);
		this->state = ParserState::InVersion;
	}
}

void V2HttpParser::setVersion(std::string&& version) {
	if (version != "HTTP/1.1") {
		onError(505, "HTTP Version not supported.");
	}
	else {
		this->version = std::move(version);
		this->state = ParserState::InHeaders;
	}
}

void V2HttpParser::addHeader(std::string&& key, std::string&& value) {
	//Excesses will be caught by the max size limit.
	if (key.length() == 0) {
		onError(400, "Missing header field name.");
	}
	else {
		//This will NOT invoke a move constructor because 'key' is still an lvalue, even if it's of an rvalue reference type.
		lastHeaderKey = key;
		this->headers.insert(std::make_pair(std::move(key), std::move(value)));
	}

	//TODO: get ContentLength
}

void V2HttpParser::extendHeader(std::string&& value) {
	if (lastHeaderKey.length() == 0) {
		onError(400, "Continuation line before any headers.");
	}
	else {
		std::string& dest = this->headers[lastHeaderKey];

		if (dest.empty()) {
			dest = std::move(value);
		}
		else {
			//Need to add a space, then the new value.
			dest.reserve(dest.length() + value.length() + 1);
			dest.append(" ", 1);
			dest.append(value);
		}
	}
}

void V2HttpParser::onBodyStart() {
	this->state = ParserState::InBody;
}

void V2HttpParser::setBody(std::string&& body) {
	this->body = std::move(body);
}

void V2HttpParser::onError(int statusCode, std::string&& reason) {	
	this->state = ParserState::Error;

	this->errorStatusCode = statusCode;
	this->errorMessage = std::move(reason);
}

void V2HttpParser::consume(std::string::iterator start, std::string::iterator end) {
	if (isError()) {
		return;
	}

	while(start != end && !isError()) { //TODO: enforce limits
		fsm.consumeOne(*start);
		++start;
	}
}

std::unique_ptr<Request> V2HttpParser::getParsed() {
	if (isError()) {
		return nullptr;
	}

	fsm.doFinalPass(); //TODO: move to a finalize() function once the split is made in v2PymlParser too

	const auto& verbIt = methodStringMapping.find(this->method);
	if (verbIt == methodStringMapping.end()) {
		onError(501, formatString("Parsing HTTP method: method %1% not recognized.", this->method));
		return nullptr;
	}
	HttpVerb verb = verbIt->second;

	std::string queryString;
	size_t queryStringStart = this->url.find('?');
	if (queryStringStart != std::string::npos) {
		queryString = this->url.substr(queryStringStart + 1);
		url.erase(queryStringStart);
	}
	urlDecode(url);

	return std::make_unique<Request>(verb, std::move(url), std::move(queryString), 1, 1, headers, std::move(body));
}
