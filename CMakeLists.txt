cmake_minimum_required(VERSION 3.6)
project(krait)

set(CMAKE_CXX_STANDARD 14)

if(NOT DEFINED DO_DEBUG_BUILD)
	set(DO_DEBUG_BUILD ON)
endif()
if (NOT DEFINED DO_STATIC_BUILD)
	set(DO_STATIC_BUILD OFF)
endif()
if(NOT DEFINED DO_PROFILE_BUILD)
	set(DO_PROFILE_BUILD OFF)
endif()

if (DO_PROFILE_BUILD)
	set(DO_DEBUG_BUILD OFF)
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

add_subdirectory(${PROJECT_SOURCE_DIR}/src)

get_directory_property(SOURCE_FILES
	DIRECTORY ${PROJECT_SOURCE_DIR}/src
	DEFINITION SOURCE_FILES)
get_directory_property(MAIN_FILES
	DIRECTORY ${PROJECT_SOURCE_DIR}/src
	DEFINITION MAIN_FILES)
get_filename_component(SOURCE_MAIN_CPP "src/main.cpp" ABSOLUTE)
get_filename_component(SOURCE_MAIN_TEST_CPP "src/main_tests.cpp" ABSOLUTE)


# message("Source files:")
# message("${SOURCE_FILES} ${MAIN_FILES}")



SET(ORIGINAL_LIB_SUFFIX ${CMAKE_FIND_LIBRARY_SUFFIXES})
SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so")
FIND_PACKAGE(PythonLibs 2.7 REQUIRED)
SET(CMAKE_FIND_LIBRARY_SUFFIXES ${ORIGINAL_LIB_SUFFIX})

include_directories(${PYTHON_INCLUDE_DIRS})

FIND_PACKAGE(Boost COMPONENTS python filesystem regex thread chrono program_options)
if(Boost_FOUND)
    INCLUDE_DIRECTORIES("${Boost_INCLUDE_DIRS}")
    SET(Boost_USE_STATIC_LIBS ${STATIC_BOOST})
    SET(Boost_USE_MULTITHREADED ON)
    SET(Boost_USE_STATIC_RUNTIME ${STATIC_BOOST})
else()
    MESSAGE(FATAL_ERROR "Unable to find correct Boost version. Did you set BOOST_ROOT?")
endif()

set(BUILD_DIR ${PROJECT_SOURCE_DIR}/build)
set(BIN_DIR ${BUILD_DIR}/bin)
set(SHARE_DIR ${BUILD_DIR}/share/krait)

add_executable(build ${SOURCE_MAIN_CPP} ${SOURCE_FILES})

target_link_libraries(build ${PYTHON_LIBRARY} ${Boost_LIBRARIES})
set_target_properties(build PROPERTIES OUTPUT_NAME ${BIN_DIR}/${CMAKE_PROJECT_NAME})

add_custom_command(TARGET build PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BIN_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILD_DIR}/share
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHARE_DIR}
)

add_custom_command(TARGET build POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/py ${SHARE_DIR}/py)
add_custom_command(TARGET build POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/globals ${SHARE_DIR}/globals)
add_custom_command(TARGET build POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/krait-cmdr ${BIN_DIR}/krait-cmdr)
add_custom_command(TARGET build POST_BUILD
    COMMAND /bin/chmod uga+xr ${BIN_DIR}/krait-cmdr)


set(TEST_DIR ${PROJECT_SOURCE_DIR}/tests)

add_executable(build_tests ${SOURCE_MAIN_TEST_CPP} ${SOURCE_FILES})
target_link_libraries(build_tests ${PYTHON_LIBRARY} ${Boost_LIBRARIES})
set_target_properties(build_tests PROPERTIES OUTPUT_NAME ${TEST_DIR}/${CMAKE_PROJECT_NAME}_tests)
set_target_properties(build_tests PROPERTIES EXCLUDE_FROM_ALL TRUE)
add_custom_command(TARGET build_tests PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TEST_DIR})
add_custom_command(TARGET build_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/py ${TEST_DIR}/py)
add_custom_command(TARGET build_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/globals ${BUILD_DIR}/globals)


install(DIRECTORY ${BUILD_DIR}/ DESTINATION ${CMAKE_INSTALL_PREFIX})
