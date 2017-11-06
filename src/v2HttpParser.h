﻿#pragma once
#include "fsmV2.h"
#include "request.h"
#include <string>

//#define DBG_DISABLE
#include "dbg.h"

class IV2HttpParser {
public:
    virtual ~IV2HttpParser() = default;
    virtual void setMethod(std::string&& method) = 0;
    virtual void setUrl(std::string&& url) = 0;
    virtual void setVersion(std::string&& version) = 0;
    virtual void addHeader(std::string&& key, std::string&& value) = 0;
    virtual void extendHeader(std::string&& value) = 0;
    virtual void onBodyStart() = 0;
    virtual void setBody(std::string&& body) = 0;
    virtual void onError(int statusCode, std::string&& reason) = 0;
};

class V2HttpFsm : public FsmV2 {
    class HttpRootTransition : public DelegatingFsmTransition {
    protected:
        IV2HttpParser** parser;

    public:
        HttpRootTransition(IV2HttpParser** parser, FsmTransition*&& base)
                : DelegatingFsmTransition(std::move(base)), parser(parser) {
        }
    };
    class HttpSetMethodTransition : public HttpRootTransition {
    public:
        HttpSetMethodTransition(IV2HttpParser** parser, FsmTransition*&& base)
                : HttpRootTransition(parser, std::move(base)) {
        }

        void execute(FsmV2& fsm) override {
            HttpRootTransition::execute(fsm);

            (*parser)->setMethod(fsm.getResetStored());
        }

        FsmTransition& getDelegateExecute() override {
            return *this;
        }
    };
    class HttpSetUrlTransition : public HttpRootTransition {
    public:
        HttpSetUrlTransition(IV2HttpParser** parser, FsmTransition*&& base)
                : HttpRootTransition(parser, std::move(base)) {
        }

        void execute(FsmV2& fsm) override {
            HttpRootTransition::execute(fsm);

            (*parser)->setUrl(fsm.getResetStored());
        }

        FsmTransition& getDelegateExecute() override {
            return *this;
        }
    };
    class HttpSetVersionTransition : public HttpRootTransition {
    public:
        HttpSetVersionTransition(IV2HttpParser** parser, FsmTransition*&& base)
                : HttpRootTransition(parser, std::move(base)) {
        }

        void execute(FsmV2& fsm) override {
            HttpRootTransition::execute(fsm);

            (*parser)->setVersion(fsm.getResetStored());
        }

        FsmTransition& getDelegateExecute() override {
            return *this;
        }
    };
    class HttpAddHeaderTransition : public HttpRootTransition {
    public:
        HttpAddHeaderTransition(IV2HttpParser** parser, FsmTransition*&& base)
                : HttpRootTransition(parser, std::move(base)) {
        }

        void execute(FsmV2& fsm) override {
            HttpRootTransition::execute(fsm);

            std::string key = fsm.popStoredString();
            std::string value = fsm.popStoredString();

            (*parser)->addHeader(std::move(key), std::move(value));
        }

        FsmTransition& getDelegateExecute() override {
            return *this;
        }
    };
    class HttpExtendHeaderTransition : public HttpRootTransition {
    public:
        HttpExtendHeaderTransition(IV2HttpParser** parser, FsmTransition*&& base)
                : HttpRootTransition(parser, std::move(base)) {
        }

        void execute(FsmV2& fsm) override {
            HttpRootTransition::execute(fsm);

            (*parser)->extendHeader(fsm.getResetStored());
        }

        FsmTransition& getDelegateExecute() override {
            return *this;
        }
    };
    class HttpOnBodyStartTransition : public HttpRootTransition {
    public:
        HttpOnBodyStartTransition(IV2HttpParser** parser, FsmTransition*&& base)
                : HttpRootTransition(parser, std::move(base)) {
        }

        void execute(FsmV2& fsm) override {
            HttpRootTransition::execute(fsm);

            (*parser)->onBodyStart();
            fsm.resetStored();
        }

        FsmTransition& getDelegateExecute() override {
            return *this;
        }
    };
    class HttpSetBodyTransition : public HttpRootTransition {
    public:
        HttpSetBodyTransition(IV2HttpParser** parser, FsmTransition*&& base)
                : HttpRootTransition(parser, std::move(base)) {
        }

        void execute(FsmV2& fsm) override {
            HttpRootTransition::execute(fsm);

            (*parser)->setBody(fsm.getResetStored());
        }

        FsmTransition& getDelegateExecute() override {
            return *this;
        }
    };
    class HttpErrorTransition : public HttpRootTransition {
        int statusCode;
        std::string reason;

    public:
        HttpErrorTransition(int statusCode, std::string&& reason, IV2HttpParser** parser, FsmTransition*&& base)
                : HttpRootTransition(parser, std::move(base)), statusCode(statusCode), reason(std::move(reason)) {
        }

        void execute(FsmV2& fsm) override {
            HttpRootTransition::execute(fsm);

            (*parser)->onError(statusCode, std::move(reason));
        }

        FsmTransition& getDelegateExecute() override {
            return *this;
        }
    };

    IV2HttpParser* parser;
    void init();

public:
    V2HttpFsm();

    void setParser(IV2HttpParser* parser) {
        this->parser = parser;
    }
};

class V2HttpParser : public IV2HttpParser {
    std::string method;
    std::string url;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;

    std::string lastHeaderKey;

    int bodyBytesLeft;
    int headerBytesLeft;

    int errorStatusCode;
    std::string errorMessage;

    enum class ParserState { InMethod, InUrl, InVersion, InHeaders, InBody, Error };
    ParserState state;

    int maxBodyLegth;
    int maxHeaderLength;

    void urlDecode(std::string& url);
    int getContentLengthFromHeaders();


    static V2HttpFsm fsm;
    static std::map<std::string, HttpVerb> methodStringMapping;

public:
    explicit V2HttpParser(int maxHeaderLength, int maxBodyLength);
    void reset();

    void setMethod(std::string&& method) override;
    void setUrl(std::string&& url) override;
    void setVersion(std::string&& version) override;
    void addHeader(std::string&& key, std::string&& value) override;
    void extendHeader(std::string&& value) override;
    void onBodyStart() override;
    void setBody(std::string&& body) override;
    void onError(int statusCode, std::string&& reason) override;

    void consume(char* start, size_t length);
    std::unique_ptr<Request> getParsed();

    bool isError() const {
        return state == ParserState::Error;
    }
    bool isFinished() const {
        return state == ParserState::InBody && bodyBytesLeft == 0;
    }
    int getErrorStatusCode() const {
        return errorStatusCode;
    }
    const std::string& getErrorMessage() const {
        return errorMessage;
    }
};
