#pragma once

#include <boost/exception/all.hpp>
#include <iostream>

typedef boost::error_info<struct tag_system_info, const char*, int> syscall_info;
struct syscall_error: virtual boost::exception, virtual std::exception {};