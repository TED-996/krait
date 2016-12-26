#pragma once
#include <string>


enum HttpVerb {
	ANY = 0,
    GET,
    HEAD,
    POST,
    PUT,
    DELETE,
    CONNECT,
    OPTIONS,
    TRACE
};


std::string httpVerbToString(HttpVerb value);

