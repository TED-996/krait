#pragma once
#include <string>


enum class HttpVerb {
    GET = 0,
    HEAD,
    POST,
    PUT,
    DELETE,
    CONNECT,
    OPTIONS,
    TRACE
};

enum class RouteVerb {
	INVALID = 0,
	GET,
	POST,
	PUT,
	DELETE,
	CONNECT,
	OPTIONS,
	TRACE,
	ANY,
	WEBSOCKET
};


RouteVerb toRouteVerb(HttpVerb verb);
RouteVerb toRouteVerb(std::string str);
std::string httpVerbToString(HttpVerb value);
std::string routeVerbToString(RouteVerb value);

