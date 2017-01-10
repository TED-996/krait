#pragma once
#include <string>
#include <vector>
#include <map>
#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>
#include "http.h"


class Route {
	bool defaultRoute;

	HttpVerb verb;
	boost::regex urlRegex;
	std::map<int, std::string> matchParameters;
	std::string targetFilename;

public:
	Route(HttpVerb verb, std::string targetFilename);
	Route(HttpVerb verb, boost::regex urlRegex, std::map<int, std::string> matchParameters, std::string file);

	bool isDefaultTarget() {
		return targetFilename == "$default$";
	}

	std::string getTarget() {
		return targetFilename;
	}


	bool isMatch(HttpVerb verb, std::string url, std::map<std::string, std::string>& outParams);

	static Route getRoute(boost::property_tree::ptree routePtree);
};


std::vector<Route> getRoutesFromFile(std::string filename);
std::vector<Route> getDefaultRoutes();
Route& getRouteMatch(std::vector<Route> routes, HttpVerb verb, std::string url,
                     std::map<std::string, std::string>& outParams);
