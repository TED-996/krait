#include"routes.h"
#include"except.h"

#define DBG_DISABLE
#include"dbg.h"

namespace b = boost;


Route::Route(RouteVerb verb, boost::optional<boost::regex> urlRegex, boost::optional<std::string> urlRaw,
             boost::optional<std::string> target, boost::optional<boost::python::object> ctrlClass)
	: verb(verb), urlRegex(urlRegex), urlRaw(urlRaw), target(target), ctrlClass(ctrlClass) {
}


const std::string& Route::getTarget(const std::string& defaultTarget) const {
	if (target == boost::none) {
		return defaultTarget;
	}
	else {
		return target.get();
	}
}

bool Route::isMatch(RouteVerb verb, std::string url, std::map<std::string, std::string>& outParams) const {
	outParams.clear();

	if (this->verb != RouteVerb::ANY && verb != this->verb) {
		return false;
	}

	if (urlRegex == boost::none && urlRaw == boost::none) {
		return true;
	}
	else if (urlRaw != boost::none && url == urlRaw.get()) {
		return true;
	}
	else if (urlRegex != boost::none && regex_match(url.c_str(), urlRegex.get())) {
		return true;
	}
	else {
		return false;
	}
}

bool Route::isMvcRoute() const {
	return (bool)ctrlClass;
}

const boost::python::object& Route::getCtrlClass() const {
	if (ctrlClass == boost::none) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("getCtrlClass called without this being a MVC route."));
	}
	
	return ctrlClass.value();
}

std::vector<Route> Route::getDefaultRoutes() {
	return std::vector<Route>{Route(RouteVerb::GET, boost::none, boost::none, boost::none, boost::none)};
}

const Route& Route::getRouteMatch(const std::vector<Route>& routes, RouteVerb verb, std::string url,
                                  std::map<std::string, std::string>& outParams) {
	for (const Route& route : routes) {
		if (route.isMatch(verb, url, outParams)) {
			return route;
		}
	}
	BOOST_THROW_EXCEPTION(routeError() <<
		stringInfoFromFormat("Error: Could not match url %1% on method %2% with any route. Is there no default route?",
			url, routeVerbToString(verb)));
}
