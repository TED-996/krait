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