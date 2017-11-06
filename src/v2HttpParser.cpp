#include "v2HttpParser.h"
#include "except.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/compare.hpp>

#define DBG_DISABLE
#include "dbg.h"
#include "dbgStopwatch.h"


V2HttpFsm V2HttpParser::fsm;
std::map<std::string, HttpVerb> V2HttpParser::methodStringMapping{{"GET", HttpVerb::GET},
    {"HEAD", HttpVerb::HEAD},
    {"POST", HttpVerb::POST},
    {"PUT", HttpVerb::PUT},
    {"DELETE", HttpVerb::DELETE},
    {"CONNECT", HttpVerb::CONNECT},
    {"OPTIONS", HttpVerb::OPTIONS},
    {"TRACE", HttpVerb::TRACE}};


V2HttpFsm::V2HttpFsm() : FsmV2(20, 0), parser(nullptr) {
    init();
}

void V2HttpFsm::init() {
    DbgStopwatch("V2HttpFsm::init");

    typedef SimpleFsmTransition Simple;
    typedef PushFsmTransition Push;
    typedef DiscardFsmTransition Discard;
    typedef AlwaysFsmTransition Always;
    typedef SkipFsmTransition Skip;
    typedef FinalFsmTransition Final;

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

    add(inMethod, new HttpSetMethodTransition(&parser, new Skip(new Simple(' ', afterMethod))));
    add(inMethod, new Always(inMethod));

    add(afterMethod, new Discard(new Always(inUrl)));

    add(inUrl, new HttpSetUrlTransition(&parser, new Skip(new Simple(' ', afterUrl))));
    add(inUrl, new Always(inUrl));

    add(afterUrl, new Always(inVersion));

    add(inVersion, new HttpSetVersionTransition(&parser, new Skip(new Simple('\r', afterVersion))));
    add(inVersion, new Always(inVersion));

    add(afterVersion, new Skip(new Simple('\n', newLine)));
    add(afterVersion,
        new HttpErrorTransition(400, "Incomplete CR-LF after HTTP version.", &parser, new Always(errorState)));

    add(newLine, new Simple('\r', newLineInCrlf));

    // If a header line starts with a space or tab, it's a continuation line for the previous header's value
    add(newLine, new Simple(' ', beforeHeaderExtension));
    add(newLine, new Simple('\t', beforeHeaderExtension));

    add(newLine, new Discard(new Always(inHeaderKey)));

    add(newLineInCrlf, new HttpOnBodyStartTransition(&parser, new Discard(new Skip(new Simple('\n', inBody)))));
    add(newLineInCrlf, new HttpErrorTransition(400, "Incomplete CR-LF after CR-LF.", &parser, new Always(errorState)));

    add(inHeaderKey, new Push(new Skip(new Simple(':', afterHeaderKey))));
    add(inHeaderKey, new Always(inHeaderKey));

    add(afterHeaderKey, new Simple('\r', inHeaderLwsInCrlf));
    add(inHeaderLwsInCrlf,
        new HttpAddHeaderTransition(&parser, new Push(new Discard(new Skip(new Simple('\n', newLine))))));
    add(inHeaderLwsInCrlf,
        new HttpErrorTransition(400, "Incomplete CR-LF in LWS before header value.", &parser, new Always(errorState)));

    add(afterHeaderKey, new Simple(' ', afterHeaderKey));
    add(afterHeaderKey, new Simple('\t', afterHeaderKey));
    add(afterHeaderKey, new Discard(new Always(inHeaderValue)));

    add(inHeaderValue, new HttpAddHeaderTransition(&parser, new Push(new Simple('\r', afterHeaderValue))));
    add(inHeaderValue, new Always(inHeaderValue));

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

    addFinalActionToMany(
        [](FsmV2& fsm) {
            BOOST_THROW_EXCEPTION(httpParseError() << stringInfo("Incomplete request. Insufficient information."));
        },
        {inMethod,
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
            inHeaderExtension});
    addFinalAction(errorState, [](FsmV2& fsm) {
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
    fsm.reset();
}


void V2HttpParser::setMethod(std::string&& method) {
    if (method.length() >= 24) {
        onError(501, "Method too large");
    } else {
        this->method = std::move(method);
        this->state = ParserState::InUrl;
    }
}

void V2HttpParser::setUrl(std::string&& url) {
    if (url.length() >= 8192) {
        onError(414, "URI Too Long");
    } else {
        this->url = std::move(url);
        this->state = ParserState::InVersion;
    }
}

void V2HttpParser::setVersion(std::string&& version) {
    if (version != "HTTP/1.1") {
        onError(505, "HTTP Version not supported.");
    } else {
        this->version = std::move(version);
        this->state = ParserState::InHeaders;
    }
}

void V2HttpParser::addHeader(std::string&& key, std::string&& value) {
    // Excesses will be caught by the max size limit.
    if (key.length() == 0) {
        onError(400, "Missing header field name.");
        return;
    }
    boost::to_lower(key);

    // This will NOT invoke a move constructor because 'key' is still an lvalue, even if it's of an rvalue reference
    // type.
    lastHeaderKey = key;

    auto existingHeader = this->headers.find(key);
    if (existingHeader == this->headers.end()) {
        this->headers.insert(std::make_pair(std::move(key), std::move(value)));
    } else {
        std::string& dest = existingHeader->second;
        dest.reserve(dest.length() + value.length() + 1);
        dest.append(",");
        dest.append(value);
    }

    if (bodyBytesLeft == 0 && lastHeaderKey == "content-length") {
        bodyBytesLeft = getContentLengthFromHeaders();
    }
}

void V2HttpParser::extendHeader(std::string&& value) {
    if (lastHeaderKey.length() == 0) {
        onError(400, "Continuation line before any headers.");
        return;
    }

    std::string& dest = this->headers[lastHeaderKey];

    if (dest.empty()) {
        dest = std::move(value);
    } else {
        // Need to add a space, then the new value.
        dest.reserve(dest.length() + value.length() + 1);
        dest.append(" ", 1);
        dest.append(value);
    }

    if (bodyBytesLeft == 0) {
        bodyBytesLeft = getContentLengthFromHeaders();
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

void V2HttpParser::consume(char* start, size_t length) {
    if (isFinished() || isError()) {
        return;
    }
    char* end = start + length;
    while (start != end && !isError() && !isFinished()) {
        if (state != ParserState::InBody) {
            headerBytesLeft--;
            if (headerBytesLeft < 0) {
                onError(400, "Request header too large.");
            }
        } else {
            bodyBytesLeft--;
            if (bodyBytesLeft < 0) {
                BOOST_THROW_EXCEPTION(serverError() << stringInfo("V2HttpParser: FSM not stopped when body is over."));
            }
        }

        fsm.consumeOne(*start);
        ++start;
    }

    if (isFinished() || isError()) {
        fsm.doFinalPass('\0');
    }
}


std::unique_ptr<Request> V2HttpParser::getParsed() {
    if (isError()) {
        return nullptr;
    }

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

    return std::make_unique<Request>(
        verb, std::move(url), std::move(queryString), 1, 1, std::move(headers), std::move(body));
}

void V2HttpParser::urlDecode(std::string& url) {
    size_t offset = 0;

    bool inEscapeSeq = false;
    int escapeValueDigits = 0;
    int escapeValue = 0;

    for (size_t i = 0; i < url.length(); i++) {
        if (!inEscapeSeq) {
            if (url[i] == '%') {
                inEscapeSeq = true;
                escapeValueDigits = 0;
                escapeValue = 0;
            } else if (offset != 0) {
                url[i - offset] = url[i];
            }
        } else {
            const char ch = url[i];
            int chValue;

            if (ch >= '0' && ch <= '9') {
                chValue = ch - '0';
            } else if (ch >= 'a' && ch <= 'f') {
                chValue = ch - 'a' + 10;
            } else if (ch >= 'A' && ch <= 'F') {
                chValue = ch - 'A' + 10;
            } else {
                // DBG_FMT("URL unquoting error: character '%1%' in string '%2%' not hexadecimal.", ch, url);
                onError(400, "URL unquoting: non-hex character");
                return;
            }

            if (escapeValueDigits == 0) {
                escapeValue = chValue << 4;
            } else {
                escapeValue |= chValue;
            }

            escapeValueDigits++;
            if (escapeValueDigits == 2) {
                //%12, str[i] was '2' => '%' is str[i - 2]
                offset += 2;

                url[i - offset] = (char) escapeValue;
                inEscapeSeq = false;
            }
        }
    }

    url.resize(url.length() - offset); // IMPORTANT: Offset and NewSize must be kept in sync!
}

int V2HttpParser::getContentLengthFromHeaders() {
    const auto& lengthHeader = this->headers.find("content-length");
    if (lengthHeader == this->headers.end()) {
        return 0;
    }

    size_t idx = 0;
    int length = std::stoi(lengthHeader->second, &idx, 10);

    if (idx != lengthHeader->second.size()) {
        BOOST_THROW_EXCEPTION(httpParseError() << stringInfo("Content-Length not int!"));
    }

    return length;
}
