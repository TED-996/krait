#pragma once
#include <string>
#include <vector>
#include <map>
#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>
#include "http.h"


class Route
{
	RouteVerb verb;
	boost::optional<boost::regex> urlRegex;
	boost::optional<std::string> urlRaw;
	boost::optional<std::string> target;

public:
	Route(RouteVerb verb,
	      boost::optional<boost::regex> urlRegex,
	      boost::optional<std::string> urlRaw,
	      boost::optional<std::string> target);

	const std::string& getTarget(const std::string& defaultTarget) const;


	bool isMatch(RouteVerb verb, std::string url, std::map<std::string, std::string>& outParams) const;

	static Route getRoute(boost::property_tree::ptree routePtree);
	static std::vector<Route> getRoutesFromFile(std::string filename);
	static std::vector<Route> getDefaultRoutes();
	static const Route& getRouteMatch(const std::vector<Route>& routes, RouteVerb verb, std::string url,
	                                  std::map<std::string, std::string>& outParams);
};
