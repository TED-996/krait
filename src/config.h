#pragma once
#include <vector>
#include "routes.h"

class Config
{
private:
	bool initialized;

	std::vector<Route> routes;
	void loadRoutes();

public:
	Config();
	void load();

	std::vector<Route>& getRoutes();
};
