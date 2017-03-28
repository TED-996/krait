#pragma once

#include <boost/filesystem.hpp>
#include <string>

boost::filesystem::path getExecRoot();
boost::filesystem::path getCreateDataRoot();
bool pathCheckExists(std::string filename);