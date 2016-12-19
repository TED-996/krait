#pragma once

#include <boost/exception/all.hpp>
#include<boost/format.hpp>
#include <string>
#include "formatHelper.h"


typedef boost::error_info<struct tag_string_info, const std::string> stringInfo;
typedef boost::error_info<struct tag_errcode_info, const int> errcodeInfo;

stringInfo stringInfoFromFormat(std::string format, ...);

struct rootException : virtual boost::exception, virtual std::exception {
	const char* what() const noexcept {
		return boost::diagnostic_information_what(*this);
	}
};

struct syscallError: virtual rootException {};
struct pythonError: virtual rootException {};
struct networkError: virtual rootException {};
struct httpParseError: virtual rootException {};
struct routeParseError: virtual rootException {};
struct routeError: virtual rootException {};
struct serverError: virtual rootException{};

struct notImplementedError: virtual rootException {};

template <typename... TArgs>
stringInfo stringInfoFromFormat(const char* fmt, TArgs&& ... args){
	return stringInfo(formatString(fmt, std::forward<TArgs>(args)...));
}
