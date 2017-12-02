#pragma once
#include "IPymlCache.h"
#include "responseSource.h"
#include <boost/filesystem/path.hpp>
#include <boost/utility/string_ref.hpp>


class SourceConverter {
    boost::filesystem::path siteRoot;
    IPymlCache& cache;

public:
    explicit SourceConverter(boost::filesystem::path siteRoot, IPymlCache& cache)
            : siteRoot(std::move(siteRoot)), cache(cache) {
    }

    std::string sourceToModuleName(boost::string_ref filename) const;
    std::string moduleNameToSource(boost::string_ref moduleName) const;

    void extend(ResponseSource& source);
};
