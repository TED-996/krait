#include <boost/property_tree/xml_parser.hpp>
#include"routes.h"
#include"except.h"

using namespace std;
using namespace boost;
using namespace boost::property_tree;


Route::Route(regex urlRegex, map<int, string> matchParameters, string targetFilename){
	this->urlRegex = urlRegex;
	this->matchParameters = matchParameters;
	this->targetFilename = targetFilename;
	this->defaultRoute = false;
}

Route::Route(){
	this->defaultRoute = true;
}

bool Route::isMatch(string url, map<string, string>& outParams) {
	outParams.clear();
	
	if (defaultRoute){
		return true;
	}
	
	cmatch matchVariables;
	if (regex_match(url.c_str(), matchVariables, urlRegex)){
		for (auto paramRequest : matchParameters){
			size_t paramIndex = paramRequest.first;
			string paramIdentifier = paramRequest.second;
			if (paramIndex >= matchVariables.size()){
				throw routeError() << stringInfoFromFormat("Error matching URL %1% with route for regex %2%: argument %3% requires position %4%,"
														   "but there are only %5% capture groups.",
														   url, urlRegex.str(), paramIdentifier, paramIndex, matchVariables.size());
			}
			
			outParams[paramIdentifier]  = string(matchVariables[paramIndex]);
		}
		
		return true;
	}
	
	return false;
}

Route Route::getRoute(ptree routePtree){
	/* 	<route>
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
	
	optional<string> defaultValue = routePtree.get_optional<string>("<xmlattr>.default");
	if (defaultValue){
		if (*defaultValue == "true"){
			return getDefaultRoute();
		}
		else{
			throw routeError() << stringInfo("Error: Found route with attribute 'default', but value is not 'true'.");
		}
	}
	try{
		string urlRegex = routePtree.get<string>("route.regex");
		string target = routePtree.get<string>("route.target");
		
		optional<ptree> captures = routePtree.get_child("route.captures");
		map<int, string> matchParameters;
		if (captures){
			for (ptree::value_type cap : *captures){
				if (cap.first != "capture"){
					throw routeError() << stringInfoFromFormat("Error: unexpected node '%1%' found.", cap.first);
				}
				int idx = cap.second.get<int>("<xmlattr>.idx");
				string identifier = cap.second.get<string>("<xmlattr>.identifier");
				
				matchParameters[idx] = identifier;
			}
		}
		
		return Route(regex(urlRegex), matchParameters, target);
	}
	catch(ptree_bad_path& ex){
		throw routeError() << stringInfoFromFormat("Error: Could not find route parameter '%1%'.", ex.path<string>());
	}
	catch(ptree_bad_data& ex){
		throw routeError() << stringInfoFromFormat("Error: Could not convert data '%1%' to the expected type.", ex.data<string>());
	}
	
}

Route Route::getDefaultRoute(){
	return Route();
}
