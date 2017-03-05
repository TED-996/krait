cmake_minimum_required(VERSION 3.6)
project(krait)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing -D_FORTIFY_SOURCE=2 -g -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security  -DNDEBUG -g -fwrapv -O2 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing -Wformat -Werror=format-security -g -fwrapv -Wall")

set(CMAKE_CXX_STANDARD 11)

add_subdirectory(src)

