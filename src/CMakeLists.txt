file(GLOB SOURCE_FILES
    "*.h"
    "*.cpp"
)
file(GLOB MAIN_FILES
    "main*.cpp"
)
file(GLOB TEST_FILES
    "*tests.cpp"
)


list(REMOVE_ITEM SOURCE_FILES ${MAIN_FILES})
list(REMOVE_ITEM SOURCE_FILES ${TEST_FILES})

message("Source files:")
message(${SOURCE_FILES} ${MAIN_FILES})

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

add_executable(build main.cpp ${SOURCE_FILES})

target_link_libraries(build ${PYTHON_LIBRARY} ${Boost_LIBRARIES})
set_target_properties(build PROPERTIES OUTPUT_NAME ${BIN_DIR}/${CMAKE_PROJECT_NAME})

add_custom_command(TARGET build PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BIN_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILD_DIR}/share
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHARE_DIR}
)

add_custom_command(TARGET build POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/py ${SHARE_DIR}/py)
add_custom_command(TARGET build POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/globals ${SHARE_DIR}/globals)
add_custom_command(TARGET build POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/krait-cmdr ${BIN_DIR}/krait-cmdr)
add_custom_command(TARGET build POST_BUILD
    COMMAND /bin/chmod uga+xr ${BIN_DIR}/krait-cmdr)


set(TEST_DIR ${PROJECT_SOURCE_DIR}/tests)

add_executable(build_tests main_tests.cpp ${SOURCE_FILES})
target_link_libraries(build_tests ${PYTHON_LIBRARY} ${Boost_LIBRARIES})
set_target_properties(build_tests PROPERTIES OUTPUT_NAME ${TEST_DIR}/${CMAKE_PROJECT_NAME}_tests)
set_target_properties(build_tests PROPERTIES EXCLUDE_FROM_ALL TRUE)
add_custom_command(TARGET build_tests PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TEST_DIR})
add_custom_command(TARGET build_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/py ${TEST_DIR}/py)
add_custom_command(TARGET build_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/globals ${BUILD_DIR}/globals)


install(DIRECTORY ${BUILD_DIR} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
