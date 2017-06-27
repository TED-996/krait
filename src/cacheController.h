#pragma once

#include<string>
#include<map>
#include<utility>
#include"regexList.h"
#include "config.h"


class CacheController
{
public:
	struct CachePragma
	{
		bool isCache:1;
		bool isStore:1;
		bool isPrivate:1;
		bool isLongTerm:1;
		bool isRevalidate:1;
	};

private:
	RegexList noStoreTargets;
	RegexList privateTargets;
	RegexList publicTargets;
	RegexList longTermTargets;

	int maxAgeDefault;
	int maxAgeLongTerm;

	std::map<std::pair<std::string, bool>, CachePragma> pragmaCache;

	bool loaded;

public:
	CacheController(Config& config);
	void load();

	CachePragma getCacheControl(std::string targetFilename, bool defaultIsStore);

	std::string getValueFromPragma(CachePragma pragma);
};
