#pragma once
#include<boost/format.hpp>

inline static std::string formatStringRecurse(boost::format& message) {
	return message.str();
}

template<typename TValue, typename... TArgs>
inline std::string formatStringRecurse(boost::format& message, TValue&& arg, TArgs&& ... args) {
	message % std::forward<TValue>(arg);
	return formatStringRecurse(message, std::forward<TArgs>(args)...);
}

template<typename... TArgs>
std::string formatString(const char* fmt, TArgs&& ... args) {
	using namespace boost::io;
	boost::format message(fmt);
	return formatStringRecurse(message, std::forward<TArgs>(args)...);
}
