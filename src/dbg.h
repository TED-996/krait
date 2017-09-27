#pragma once

//#define DBG_DISABLE
#define DBG_ALL

#ifdef DBG_ALL

#define DBG
#define DBG_FMT

#endif


#ifdef DBG

#ifndef DBG_DISABLE
#include<iostream>

template<typename T>
inline void _DBG(const T& message) {
	std::cout << message << std::endl;
}
#undef DBG
#define DBG(message) _DBG(message)

#else

#undef DBG
#define DBG(message)

#endif

#endif

#ifdef DBG_FMT

#ifndef DBG_DISABLE
#include<iostream>
#include "formatHelper.h"

template<typename... TArgs>
inline void _DBG_FMT(const char* fmt, TArgs&&... args) {
	DBG(formatString(fmt, std::forward<TArgs>(args)...));
}
#undef DBG_FMT
#define DBG_FMT(fmt, ...) _DBG_FMT(fmt, __VA_ARGS__)

#else

#undef DBG_FMT
#define DBG_FMT(fmt, ...)

#endif

#endif

