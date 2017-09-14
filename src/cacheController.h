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
	const RegexList& noStoreTargets;
	const RegexList& privateTargets;
	const RegexList& publicTargets;
	const RegexList& longTermTargets;

	std::string filenameRoot;

	int maxAgeDefault;
	int maxAgeLongTerm;

	std::map<std::pair<std::string, bool>, CachePragma> pragmaCache;
public:
	CacheController(Config& config, std::string filenameRoot);

	CachePragma getCacheControl(std::string targetFilename, bool defaultIsStore);

	std::string getValueFromPragma(CachePragma pragma);
};
