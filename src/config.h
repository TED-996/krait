#pragma once
#include <vector>
#include "routes.h"
#include "regexList.h"

class Config
{
private:
	std::vector<Route> routes;
	RegexList noStoreTargets;
	RegexList privateTargets;
	RegexList publicTargets;
	RegexList longTermTargets;

	int maxAgeDefault;
	int maxAgeLongTerm;
	
	void loadRoutes();
	void loadCacheConfig();
public:
	Config();

	const std::vector<Route>& getRoutes() const {
		return routes;
	}

	const RegexList& getNoStoreTargets() const {
		return noStoreTargets;
	}

	const RegexList& getPrivateTargets() const {
		return privateTargets;
	}

	const RegexList& getPublicTargets() const {
		return publicTargets;
	}

	const RegexList& getLongTermTargets() const {
		return longTermTargets;
	}

	int getMaxAgeDefault() const {
		return maxAgeDefault;
	}

	int getMaxAgeLongTerm() const {
		return maxAgeLongTerm;
	}
};
