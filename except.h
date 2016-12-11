#pragma once

#include <boost/exception/all.hpp>
#include <iostream>
#include <string>

typedef boost::error_info<struct tag_string_info, const std::string> stringInfo;
typedef boost::error_info<struct tag_errcode_info, int> errcodeInfo;

struct syscallError: virtual boost::exception, virtual std::exception {};
struct pythonError: virtual boost::exception, virtual std::exception {};