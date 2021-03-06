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

find_package(PythonLibs 2.7 REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})

FIND_PACKAGE(Boost COMPONENTS python unit_test_framework filesystem regex thread chrono program_options)
IF(Boost_FOUND)
    INCLUDE_DIRECTORIES("${Boost_INCLUDE_DIRS}")
    SET(Boost_USE_STATIC_LIBS OFF)
    SET(Boost_USE_MULTITHREADED ON)
    SET(Boost_USE_STATIC_RUNTIME OFF)

ELSEIF(NOT Boost_FOUND)
    MESSAGE(FATAL_ERROR "Unable to find correct Boost version. Did you set BOOST_ROOT?")
ENDIF()

set(BUILD_DIR ${PROJECT_SOURCE_DIR}/build)

add_executable(cmdr main_cmdr.cpp commander.cpp utils.cpp path.cpp)
target_link_libraries(cmdr ${PYTHON_LIBRARY} ${Boost_LIBRARIES})
set_target_properties(cmdr PROPERTIES OUTPUT_NAME ${BUILD_DIR}/krait-cmdr)


add_executable(build main.cpp ${SOURCE_FILES})
add_dependencies(build cmdr)
target_link_libraries(build ${PYTHON_LIBRARY} ${Boost_LIBRARIES})
set_target_properties(build PROPERTIES OUTPUT_NAME ${BUILD_DIR}/${CMAKE_PROJECT_NAME})
add_custom_command(TARGET build PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILD_DIR})
add_custom_command(TARGET build POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/py ${BUILD_DIR}/py)
add_custom_command(TARGET build POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/globals ${BUILD_DIR}/globals)

set(TEST_DIR ${PROJECT_SOURCE_DIR}/tests)


add_executable(build_tests main_tests.cpp ${SOURCE_FILES})
target_link_libraries(build_tests ${PYTHON_LIBRARY} ${Boost_LIBRARIES})
set_target_properties(build_tests PROPERTIES OUTPUT_NAME ${TEST_DIR}/${CMAKE_PROJECT_NAME}_tests)
set_target_properties(build_tests PROPERTIES EXCLUDE_FROM_ALL TRUE)
add_custom_command(TARGET build_tests PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TEST_DIR})
add_custom_command(TARGET build_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/py ${TEST_DIR}/py)
add_custom_command(TARGET build POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/globals ${BUILD_DIR}/globals)

install(TARGETS build cmdr RUNTIME DESTINATION bin)
