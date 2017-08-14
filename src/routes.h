#pragma once
#include <string>
#include <vector>
#include <map>
#include <boost/optional.hpp>
#include <boost/regex.hpp>
#include <boost/python.hpp>
#include "http.h"


class Route
{
	RouteVerb verb;
	boost::optional<boost::regex> urlRegex;
	boost::optional<std::string> urlRaw;
	boost::optional<std::string> target;
	boost::optional<boost::python::object> ctrlClass;

public:
	Route(RouteVerb verb,
	      const boost::optional<boost::regex>& urlRegex,
	      const boost::optional<std::string>& urlRaw,
	      const boost::optional<std::string>& target,
		  const boost::optional<boost::python::object>& ctrlClass);

	const std::string& getTarget(const std::string& defaultTarget) const;
	bool isMvcRoute() const;
	const boost::python::object& getCtrlClass() const;

	bool isMatch(RouteVerb verb, const std::string& url, std::map<std::string, std::string>& outParams) const;

	static std::vector<Route> getDefaultRoutes();
	static const Route& getRouteMatch(const std::vector<Route>& routes, RouteVerb verb, const std::string& url,
	                                  std::map<std::string, std::string>& outParams);
};
