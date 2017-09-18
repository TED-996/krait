#pragma once

#include <boost/exception/all.hpp>
#include<boost/format.hpp>
#include <string>
#include "formatHelper.h"


typedef boost::error_info<struct tag_string_info, const std::string> stringInfo;
typedef boost::error_info<struct tag_errcode_info, const int> errcodeInfo;
typedef boost::error_info<struct tag_pycode_info, const std::string> pyCodeInfo;
typedef boost::error_info<struct tag_pyerr_info, const std::string> pyErrorInfo;
typedef boost::error_info<struct tag_origin_method_info, const std::string> originCallInfo;
typedef boost::error_info<struct tag_ssl_info, const std::string> sslErrorInfo;

pyErrorInfo getPyErrorInfo();
errcodeInfo errcodeInfoDef();

struct rootException : virtual boost::exception, virtual std::exception
{
	const char* what() const noexcept override {
		return boost::diagnostic_information_what(*this);
	}
};

struct syscallError: virtual rootException
{
};

struct pythonError: virtual rootException
{
};

struct networkError: virtual rootException
{
};

struct httpParseError: virtual rootException
{
};

struct routeParseError: virtual rootException
{
};

struct routeError: virtual rootException
{
};

struct notFoundError : virtual rootException
{
};

struct serverError: virtual rootException
{
};

struct cmdrError: virtual rootException
{
};

struct pymlError: virtual rootException
{
};

struct notImplementedError: virtual rootException
{
};

struct sslError : virtual rootException
{
};

struct configError : virtual rootException
{
};

template<typename... TArgs>
stringInfo stringInfoFromFormat(const char* fmt, TArgs&& ... args) {
	return stringInfo(formatString(fmt, std::forward<TArgs>(args)...));
}
