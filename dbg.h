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
#define DBG(message) std::cout << message << std::endl
#else
#undef DBG
#define DBG(message)
#endif

#endif

#ifdef DBG_FMT

#ifndef DBG_DISABLE
#include<iostream>
#include "formatHelper.h"

#undef DBG_FMT
#define DBG_FMT(format, ...) std::cout << formatString(format, __VA_ARGS__) << std::endl
#else
#undef DBG_FMT
#define DBG_FMT(format, ...)
#endif

#endif

