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

struct syscallError: public rootException
{
};

struct pythonError: public rootException
{
};

struct networkError: public rootException
{
};

struct httpParseError: public rootException
{
};

struct routeParseError: public rootException
{
};

struct routeError: public rootException
{
};

struct notFoundError : public rootException
{
};

struct serverError: public rootException
{
};

struct cmdrError: public rootException
{
};

struct pymlError: public rootException
{
};

struct notImplementedError: public rootException
{
};

struct sslError : public rootException
{
};

struct configError : public rootException
{
};

struct siteError : public rootException
{
};

template<typename... TArgs>
stringInfo stringInfoFromFormat(const char* fmt, TArgs&& ... args) {
	return stringInfo(formatString(fmt, std::forward<TArgs>(args)...));
}
