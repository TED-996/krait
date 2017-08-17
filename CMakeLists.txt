cmake_minimum_required(VERSION 3.6)
project(krait)

set(CMAKE_CXX_STANDARD 14)

set(DO_DEBUG_BUILD ON)

if(DO_DEBUG_BUILD)
# Debug flags
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing -Wformat -Werror=format-security -fwrapv -Wall -g -O0")
	set(STATIC_BOOST OFF)
else()
	# Release flags
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing -D_FORTIFY_SOURCE=2 -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security -fwrapv -O3 -Wall")
	set(STATIC_BOOST ON)
endif()

add_subdirectory(src)
