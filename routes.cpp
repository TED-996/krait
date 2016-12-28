#include <boost/property_tree/xml_parser.hpp>
#include"routes.h"
#include"except.h"

#define DBG_DISABLE
#include"dbg.h"

using namespace std;
using namespace boost;
using namespace boost::property_tree;


Route::Route(HttpVerb verb, regex urlRegex, map<int, string> matchParameters, string targetFilename){
	this->verb = verb;
	this->urlRegex = urlRegex;
	this->matchParameters = matchParameters;
	this->targetFilename = targetFilename;
	this->defaultRoute = false;
}

Route::Route(HttpVerb verb, string targetFilename){
	this->defaultRoute = true;
	
	this->verb = verb;
	this->targetFilename = targetFilename;
	this->matchParameters = map<int, string>();
}

bool Route::isMatch(HttpVerb verb, string url, map<string, string>& outParams) {
	outParams.clear();
	
	if (this->verb != HttpVerb::ANY && verb != this->verb){
		return false;
	}
	if (defaultRoute){
		return true;
	}
	
	cmatch matchVariables;
	if (regex_match(url.c_str(), matchVariables, urlRegex)){
		for (auto paramRequest : matchParameters){
			size_t paramIndex = paramRequest.first;
			string paramIdentifier = paramRequest.second;
			if (paramIndex >= matchVariables.size()){
				BOOST_THROW_EXCEPTION(routeError() << stringInfoFromFormat("Error matching URL %1% with route for regex %2%: argument %3% "
																		   "requires position %4%, but there are only %5% capture groups.",
																		   url, urlRegex.str(), paramIdentifier, paramIndex, matchVariables.size()));
			}
			
			outParams[paramIdentifier]  = string(matchVariables[paramIndex]);
		}
		
		return true;
	}
	
	return false;
}

Route Route::getRoute(ptree routePtree){
	/* 	<route>
	 * 		<verb>...</verb>
	 * 		<regex>...</regex>
	 * 		<captures>
	 * 			<capture idx="..." identifier="..."/>
	 * 			<capture idx="..." identifier="..."/>
	 * 		</captures>
	 * 		
	 * 		<target>...</target>
	 * 	</route>
	 * 
	 * OR
	 * 
	 * 	<route default="true"/>
	 */
	
	map<string, HttpVerb> stringVerbMapping = {
		{string("ANY"), HttpVerb::ANY},
		{string("GET"), HttpVerb::GET},
		{string("HEAD"), HttpVerb::HEAD},
		{string("POST"), HttpVerb::POST},
		{string("PUT"), HttpVerb::PUT},
		{string("DELETE"), HttpVerb::DELETE},
		{string("CONNECT"), HttpVerb::CONNECT},
		{string("OPTIONS"), HttpVerb::OPTIONS},
		{string("TRACE"), HttpVerb::TRACE}
	};
	
	string verbStr = routePtree.get<string>("verb", "GET");
	DBG_FMT("got verb %1%", verbStr);
	
	auto verbValIt = stringVerbMapping.find(verbStr);
	if (verbValIt == stringVerbMapping.end()){
		BOOST_THROW_EXCEPTION(routeParseError() << stringInfoFromFormat("Error: HTTP verb %1% not recognized.", verbStr));
	}
	
	string target = routePtree.get<string>("target", "$default$");
	DBG_FMT("got target %1%", target);
	
	string defaultValue = routePtree.get<string>("<xmlattr>.default", "false");
	DBG_FMT("got default %1%", defaultValue);
	
	if (defaultValue != "false"){
		if (defaultValue == "true"){
			return Route(verbValIt->second, target);
		}
		else{
			BOOST_THROW_EXCEPTION(routeParseError() << stringInfo("Error: Found route with attribute 'default', but value is not 'true'."));
		}
	}
	try{
		string urlRegex = routePtree.get<string>("regex");
		DBG_FMT("got url %1%", urlRegex);
		
		auto captures = routePtree.get_child_optional("captures");
		map<int, string> matchParameters;
		DBG_FMT("got captures: %1%", bool(captures));
		if (captures){
			for (ptree::value_type const& cap : *captures){
				if (cap.first != "capture"){
					BOOST_THROW_EXCEPTION(routeParseError() << stringInfoFromFormat("Error: unexpected node '%1%' found.", cap.first));
				}
				int idx = cap.second.get<int>("<xmlattr>.idx");
				string identifier = cap.second.get<string>("<xmlattr>.identifier");
				
				matchParameters[idx] = identifier;
			}
		}
		
		DBG("got route okay");
		
		return Route(verbValIt->second, regex(urlRegex), matchParameters, target);
	}
	catch(ptree_bad_path& ex){
		BOOST_THROW_EXCEPTION(routeParseError() << stringInfoFromFormat("Error: Could not find route parameter '%1%'.", ex.path<string>()));
	}
	catch(ptree_bad_data& ex){
		BOOST_THROW_EXCEPTION(routeParseError() << stringInfoFromFormat("Error: Could not convert data '%1%' to the expected type.", ex.data<string>()));
	}
}


vector<Route> getRoutesFromFile(string filename){
	/*	<routes>
	 * 		<route>...</route>
	 * 		...
	 * 	</routes>
	 * 
	 */
	ptree routesRoot;
	try{
		read_xml(filename, routesRoot);
	}
	catch(xml_parser_error& ex){
		BOOST_THROW_EXCEPTION(routeParseError() << stringInfoFromFormat("Error parsing routes file '%1%. Additional data:\n%2%", filename, ex.what()));
	}
	
	try{
		routesRoot = routesRoot.get_child("routes");
	}
	catch(ptree_error&){
		BOOST_THROW_EXCEPTION(routeParseError() << stringInfo("Error parsing routes: root is not <routes>"));
	}
	
	vector<Route> results;
	for (ptree::value_type const& rt : routesRoot){
		DBG("one route");
		if (rt.first != "route"){
			BOOST_THROW_EXCEPTION(routeParseError() << stringInfo("Error parsing routes:: one of <routes>'s children is not <route>."));
		}
		try{
			results.push_back(Route::getRoute(rt.second));
		}
		catch(routeParseError){
			throw;
		}
	}
	return results;
}

Route& getRouteMatch(vector<Route> routes, HttpVerb verb, string url, map<string, string>& outParams){
	for (Route& route : routes){
		if (route.isMatch(verb, url, outParams)){
			return route;
		}
	}
	BOOST_THROW_EXCEPTION(routeError() << stringInfoFromFormat("Error: Could not match url %1% with any route! Is there no default route?", url));
}
