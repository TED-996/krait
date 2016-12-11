cmake_minimum_required(VERSION 3.6)
project(krait)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing -D_FORTIFY_SOURCE=2 -g -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security  -DNDEBUG -g -fwrapv -O2 -Wall")

set(CMAKE_CXX_STANDARD 11)

file(GLOB SOURCE_FILES
    "*.h"
    "*.cpp"
)

find_package(PythonLibs 2.7 REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})

FIND_PACKAGE(Boost 1.45.0)
IF(Boost_FOUND)
    INCLUDE_DIRECTORIES("${Boost_INCLUDE_DIRS}" "/usr/include/python2.6")
    SET(Boost_USE_STATIC_LIBS OFF)
    SET(Boost_USE_MULTITHREADED ON)
    SET(Boost_USE_STATIC_RUNTIME OFF)
    FIND_PACKAGE(Boost 1.45.0 COMPONENTS python)

ELSEIF(NOT Boost_FOUND)
    MESSAGE(FATAL_ERROR "Unable to find correct Boost version. Did you set BOOST_ROOT?")
ENDIF()

set(BUILD_DIR ${PROJECT_SOURCE_DIR}/build)

add_executable(build ${SOURCE_FILES})
target_link_libraries(build ${PYTHON_LIBRARY} ${Boost_LIBRARIES})
set_target_properties(build PROPERTIES OUTPUT_NAME ${BUILD_DIR}/${CMAKE_PROJECT_NAME})
add_custom_command(TARGET build PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILD_DIR})


add_custom_target(run
    COMMAND ./${CMAKE_PROJECT_NAME}
    DEPENDS build
    WORKING_DIRECTORY ${BUILD_DIR})
