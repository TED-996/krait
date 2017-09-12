#include "config.h"
#include "pythonModule.h"
#include "except.h"

#include "dbg.h"

namespace bp = boost::python;

void Config::loadRoutes() {
	try {
		bp::object pyRoutes = PythonModule::config().getGlobalVariable("routes");
		if (pyRoutes.is_none() || bp::len(pyRoutes) == 0) {
			routes = Route::getDefaultRoutes();
		}
		else {
			routes.clear();
			long len = bp::len(pyRoutes);
			for (int i = 0; i < len; i++) {
				routes.push_back(static_cast<Route>(bp::extract<Route>(pyRoutes[i])));
			}
		}
	}
	catch (bp::error_already_set const&) {
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
