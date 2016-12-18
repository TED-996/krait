#pragma once
#include <string>
#include <vector>
#include <map>
#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>


class Route {
	bool defaultRoute;
	
	boost::regex urlRegex;
	std::map<int, std::string> matchParameters;
	std::string targetFilename;
	
	Route();
public:
	Route(boost::regex urlRegex, std::map<int, std::string> matchParameters, std::string file);
	
	
	bool isMatch(std::string url, std::map<std::string, std::string>& outParams);
	
	static Route getRoute(boost::property_tree::ptree routePtree);
	static Route getDefaultRoute();
};


std::vector<Route> getRoutesFromFile(std::string filename);
Route getRouteMatch(std::vector<Route>, std::map<std::string, std::string>& outParams);
