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
#include "formatHelper.h"

#undef DBG_FMT
#define DBG_FMT(format, ...) std::cout << formatString(format, __VA_ARGS__) << std::endl

#endif

#endif
