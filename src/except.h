#pragma once

#include <boost/exception/all.hpp>
#include <boost/format.hpp>
#include <boost/python/object.hpp>
#include <boost/io/ios_state.hpp>
#include <string>
#include "formatHelper.h"


typedef boost::error_info<struct tag_string_info, const std::string> stringInfo;
typedef boost::error_info<struct tag_errcode_info, const int> errcodeInfo;
typedef boost::error_info<struct tag_pycode_info, const std::string> pyCodeInfo;
typedef boost::error_info<struct tag_pyerr_info, const std::string> pyErrorInfo;
typedef boost::error_info<struct tag_origin_method_info, const std::string> originCallInfo;
typedef boost::error_info<struct tag_ssl_info, const std::string> sslErrorInfo;

errcodeInfo errcodeInfoDef();

template<typename T>
struct ErrorInfoWrapper
{
private:
	T object;

public:
	template<typename... TArgs>
	ErrorInfoWrapper(TArgs&&... ctorArgs) : object(std::forward<TArgs>(ctorArgs)...){}

	// Intentionally implicit.
	operator T() const {
		return object;
	}

	const T& operator()() const {
		return object;
	}
};

template<typename T>
std::ostream& operator<<(std::ostream& os, const ErrorInfoWrapper<T> wrapper)
{
	boost::io::ios_flags_saver ifs(os);
	return os << "[ErrorInfoWrapper<T> at " << std::hex << &(wrapper()) << "]";
}

typedef boost::error_info<struct tag_py_exc_type_info, ErrorInfoWrapper<boost::python::object>> pyExcTypeInfo;


template<typename... TArgs>
stringInfo stringInfoFromFormat(const char* fmt, TArgs&& ... args) {
	return stringInfo(formatString(fmt, std::forward<TArgs>(args)...));
}


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

struct compileError : public rootException
{
};

pythonError getPythonError();
