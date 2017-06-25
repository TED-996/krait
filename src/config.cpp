#include "config.h"
#include "pythonModule.h"
#include "except.h"

#include "dbg.h"

namespace bp = boost::python;

void Config::loadRoutes() {
	try {
		DBG("getting routes list");
		bp::object pyRoutes = PythonModule::config.getGlobalVariable("routes");
		DBG("route list got");
		if (pyRoutes.is_none()) {
			routes = Route::getDefaultRoutes();
		}
		else {
			routes.clear();
			DBG("pre len(pyRoutes)");
			long len = bp::len(pyRoutes);
			DBG("pre for");
			for (int i = 0; i < len; i++) {
				DBG("extracting one route");
				routes.push_back(static_cast<Route>(bp::extract<Route>(pyRoutes[i])));
			}
			DBG("post for");
		}
	}
	catch (bp::error_already_set const&) {
		DBG("Python error in loadRoutes!");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("loadRoutes"));
	}
}

Config::Config() {
	initialized = false;
	routes.clear();
}

void Config::load() {
	loadRoutes();
	initialized = true;
}

std::vector<Route>& Config::getRoutes() {
	if (!initialized) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Configuration not initialized, you must call load() at least once."));
	}

	return this->routes;
}
