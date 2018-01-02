#pragma once
#include "IPymlFile.h"

class IPymlCache {
public:
    virtual ~IPymlCache() = default;
    virtual const IPymlFile& get(const std::string& filename) = 0;
};
