cmake_minimum_required(VERSION 3.6)
project(krait)

set(CMAKE_CXX_STANDARD 14)

if(NOT DEFINED DO_DEBUG_BUILD)
	set(DO_DEBUG_BUILD ON)
endif()
if (NOT DEFINED DO_STATIC_BUILD)
	set(DO_STATIC_BUILD OFF)
endif()

if(DO_DEBUG_BUILD)
	# Debug flags
	if(NOT TOOLCHAIN_CXX_FLAGS)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing -Wformat -Werror=format-security -fwrapv -Wall -g -O0")
	endif()

	SET(CMAKE_BUILD_TYPE Debug)
else()
	# Release flags
	if(NOT TOOLCHAIN_CXX_FLAGS)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing -D_FORTIFY_SOURCE=2 -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security -fwrapv -O3 -Wall")
	endif()

	SET(CMAKE_BUILD_TYPE MinSizeRel)
	SET(CMAKE_CXX_FLAGS_MINSIZEREL "-s")
endif()

if(DO_STATIC_BUILD)
	set(STATIC_BOOST ON)
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	SET(BUILD_SHARED_LIBRARIES OFF)
else()
	set(STATIC_BOOST OFF)
	# TODO: other flagS?
endif()

add_subdirectory(src)
