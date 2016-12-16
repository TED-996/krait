#pragma once

#define DBG_ALL

#ifndef DBG_DISABLE

	#ifdef DBG_ALL

	#define DBG
	#define DBG_FMT

	#endif


	#ifdef DBG

	#include<iostream>

	#undef DBG
	#define DBG(message) std::cout << message << std::endl

	#endif

	#ifdef DBG_FMT

	#include<iostream>
	#include<boost/format.hpp>
	
	inline static std::string formatStringRecurse(boost::format& message)
	{
		return message.str();
	}
	
	template <typename TValue, typename... TArgs>
	std::string formatStringRecurse(boost::format& message, TValue&& arg, TArgs&&... args)
	{
		message % std::forward<TValue>(arg);
		return formatStringRecurse(message, std::forward<TArgs>(args)...);
	}
	
	template <typename... TArgs>
	std::string formatString(const char* fmt, TArgs&&... args)
	{
		using namespace boost::io;
		boost::format message(fmt);
		return formatStringRecurse(message, std::forward<TArgs>(args)...);
	}

	#undef DBG_FMT
	#define DBG_FMT(format, ...) std::cout << formatString(format, __VA_ARGS__) << std::endl

	#endif

#endif