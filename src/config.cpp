﻿#include "config.h"
#include "pythonModule.h"
#include "except.h"

#include "dbg.h"

namespace bp = boost::python;

Config::Config() {
	loadRoutes();
	loadCacheConfig();
}

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

void Config::loadCacheConfig() {
	noStoreTargets = RegexList::fromPythonObject(PythonModule::config().getGlobalVariable("cache_no_store"));
	privateTargets = RegexList::fromPythonObject(PythonModule::config().getGlobalVariable("cache_private"));
	publicTargets = RegexList::fromPythonObject(PythonModule::config().getGlobalVariable("cache_public"));
	longTermTargets = RegexList::fromPythonObject(PythonModule::config().getGlobalVariable("cache_long_term"));

	maxAgeDefault = bp::extract<int>(PythonModule::config().getGlobalVariable("cache_max_age_default"));
	maxAgeLongTerm = bp::extract<int>(PythonModule::config().getGlobalVariable("cache_max_age_long_term"));
}
