#include <boost/property_tree/json_parser.hpp>
#include"routes.h"
#include"except.h"

#define DBG_DISABLE
#include"dbg.h"

using namespace std;
using namespace boost;
using namespace boost::property_tree;




Route::Route(RouteVerb verb, boost::optional<boost::regex> urlRegex, boost::optional<std::string> urlRaw,
             boost::optional<std::string> target)
	: verb(verb), urlRegex(urlRegex), urlRaw(urlRaw), target(target){
}


const std::string &Route::getTarget(const std::string& defaultTarget) const {
	if (target == boost::none){
		return defaultTarget;
	}
	else{
		return target.get();
	}
}

bool Route::isMatch(RouteVerb verb, string url, map<string, string>& outParams) const {
	outParams.clear();

	if (this->verb != RouteVerb::ANY && verb != this->verb) {
		return false;
	}

	if (urlRegex == boost::none && urlRaw == boost::none){
		return true;
	}
	else if (urlRaw != boost::none && url == urlRaw.get()){
		return true;
	}
	else if (urlRegex != boost::none && regex_match(url.c_str(), urlRegex.get())) {
		return true;
	}
	else {
		return false;
	}
}

Route Route::getRoute(ptree routePtree) {
	try {
		string verbStr = routePtree.get<string>("verb", "GET");
		DBG_FMT("got verb %1%", verbStr);

		RouteVerb verb = toRouteVerb(verbStr);
		if (verb == RouteVerb::INVALID) {
			BOOST_THROW_EXCEPTION(
					routeParseError() << stringInfoFromFormat("Error: HTTP verb %1% not recognized.", verbStr));
		}

		string target = routePtree.get<string>("target", "");
		DBG_FMT("got target %1%", target);

		string url = routePtree.get<string>("url", "");
		string regex = routePtree.get<string>("regex", "");

		boost::optional<string> optionalTarget = boost::none;
		boost::optional<string> optionalUrl = boost::none;
		boost::optional<boost::regex> optionalRegex = boost::none;

		if (target != "") {
			optionalTarget = target;
		}
		if (url != "") {
			optionalUrl = url;
		}
		if (regex != "") {
			optionalRegex = boost::regex(regex);
		}

		return Route(verb, optionalRegex, optionalUrl, optionalTarget);
	}
	catch (ptree_bad_path &ex) {
		BOOST_THROW_EXCEPTION(routeParseError() << stringInfoFromFormat("Error: Could not find route parameter '%1%'.",
		                                                                ex.path<string>()));
	}
	catch (ptree_bad_data &ex) {
		BOOST_THROW_EXCEPTION(routeParseError() <<
		                                        stringInfoFromFormat(
				                                        "Error: Could not convert data '%1%' to the expected type.",
				                                        ex.data<string>()));
	}
}

vector<Route> Route::getRoutesFromFile(string filename) {
	/*  <routes>
	 *      <route>...</route>
	 *      ...
	 *  </routes>
	 *
	 */
	ptree routesRoot;
	try {
		read_json(filename, routesRoot);
	}
	catch (json_parser_error& ex) {
		BOOST_THROW_EXCEPTION(
				routeParseError() << stringInfoFromFormat("Error parsing routes file '%1%. Additional data:\n%2%",
				filename, ex.what()));
	}

	vector<Route> results;
	for (ptree::value_type const& rt : routesRoot) {
		DBG("one route");
		if (rt.first != "") {
			BOOST_THROW_EXCEPTION(routeParseError() <<
								  stringInfo("Error parsing routes: Routes JSON not an array."));
		}
		try {
			results.push_back(Route::getRoute(rt.second));
		}
		catch (routeParseError) {
			throw;
		}
	}
	return results;
}

vector<Route> Route::getDefaultRoutes(){
	return vector<Route> {Route(RouteVerb::GET, boost::none, boost::none, boost::none)};
}

const Route& Route::getRouteMatch(const vector<Route> routes, RouteVerb verb, string url, map<string, string>& outParams) {
	for (const Route& route : routes) {
		if (route.isMatch(verb, url, outParams)) {
			return route;
		}
	}
	BOOST_THROW_EXCEPTION(routeError() <<
						  stringInfoFromFormat("Error: Could not match url %1% on method %2% with any route. Is there no default route?",
											   url, routeVerbToString(verb)));
}
