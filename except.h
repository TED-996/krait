#pragma once

#include <boost/exception/all.hpp>
#include <iostream>

typedef boost::error_info<struct tag_string_info, const char*> string_info;
typedef boost::error_info<struct tag_errcode_info, int> errcode_info;

struct syscall_error: virtual boost::exception, virtual std::exception {};