#pragma once

class PymlFile;

class IPymlCache {
public:
	virtual const PymlFile* get(std::string filename) = 0;
};