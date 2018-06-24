#include "http.h"
#include <map>
#include <string>


RouteVerb toRouteVerb(HttpVerb verb) {
    if (verb == HttpVerb::GET) {
        return RouteVerb::GET;
    }
    if (verb == HttpVerb::HEAD) {
        return RouteVerb::GET;
    }
    if (verb == HttpVerb::POST) {
        return RouteVerb::POST;
    }
    if (verb == HttpVerb::PUT) {
        return RouteVerb::PUT;
    }
    if (verb == HttpVerb::DELETE) {
        return RouteVerb::DELETE;
    }
    if (verb == HttpVerb::CONNECT) {
        return RouteVerb::CONNECT;
    }
    if (verb == HttpVerb::TRACE) {
        return RouteVerb::TRACE;
    } else {
        return RouteVerb::INVALID;
    }
}

std::string httpVerbToString(HttpVerb value) {
    const char* verbStrings[]{"GET", "HEAD", "POST", "PUT", "DELETE", "CONNECT", "OPTIONS", "TRACE"};

    return std::string(verbStrings[(int) value]);
}

std::string routeVerbToString(RouteVerb value) {
    const char* verbStrings[]{
        "INVALID", "GET", "POST", "PUT", "DELETE", "CONNECT", "OPTIONS", "TRACE", "ANY", "WEBSOCKET"};

    return std::string(verbStrings[(int) value]);
}

RouteVerb toRouteVerb(std::string str) {
    std::map<std::string, RouteVerb> stringVerbMapping = {{"GET", RouteVerb::GET},
        {"POST", RouteVerb::POST},
        {"PUT", RouteVerb::PUT},
        {"DELETE", RouteVerb::DELETE},
        {"CONNECT", RouteVerb::CONNECT},
        {"OPTIONS", RouteVerb::OPTIONS},
        {"TRACE", RouteVerb::TRACE},
        {"ANY", RouteVerb::ANY},
        {"WEBSOCKET", RouteVerb::WEBSOCKET}};

    const auto it = stringVerbMapping.find(str);
    if (it == stringVerbMapping.end()) {
        return RouteVerb::INVALID;
    } else {
        return it->second;
    }
}
