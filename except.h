#pragma once

#include <boost/exception/all.hpp>
#include <iostream>
#include <string>

typedef boost::error_info<struct tag_string_info, const std::string> stringInfo;
typedef boost::error_info<struct tag_errcode_info, const int> errcodeInfo; 


struct rootException : virtual boost::exception, virtual std::exception {
	const char* what() const noexcept {
		return boost::diagnostic_information_what(*this);
	}
};

struct syscallError: virtual rootException {};
struct pythonError: virtual rootException {};
struct networkError: virtual rootException {};
struct parseError: virtual rootException {};

struct notImplementedError: virtual rootException {};
