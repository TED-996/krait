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

#undef DBG
template<typename T>
inline void DBG(const T& message) {
	std::cout << message << std::endl;
}

#else

#undef DBG
template<typename T>
inline void DBG(const T& message) {
}

#endif

#endif

#ifdef DBG_FMT

#ifndef DBG_DISABLE
#include<iostream>
#include "formatHelper.h"

#undef DBG_FMT
template<typename... TArgs>
inline void DBG_FMT(const char* fmt, TArgs&&... args) {
	DBG(formatString(fmt, std::forward<TArgs>(args)...));
}

#else

#undef DBG_FMT
template<typename... TArgs>
inline void DBG_FMT(const char* fmt, TArgs&&... args) {
}

#endif

#endif
