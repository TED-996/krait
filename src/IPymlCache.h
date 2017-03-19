#pragma once
#include "IPymlFile.h"

class IPymlCache {
public:
	virtual const IPymlFile* get(std::string filename) = 0;
};