# SPDX-License-Identifier: MIT
# Copyright 2011, SIL International, All rights reserved.
project(testexamples)

include_directories(../../src)
INCLUDE(CheckTypeSize)

CHECK_TYPE_SIZE(size_t SIZEOF_SIZE_T)
add_definitions(-DSIZEOF_SIZE_T=${SIZEOF_SIZE_T})

if  (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -DUNICODE)
    add_custom_target(${PROJECT_NAME}_copy_dll ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${graphite2_core_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}graphite2${CMAKE_SHARED_LIBRARY_SUFFIX} ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
    add_dependencies(${PROJECT_NAME}_copy_dll graphite2 simple features clusters linebreak)
endif()

macro(test_example TESTNAME SRCFILE)
    add_executable(${TESTNAME} ${SRCFILE})
    set_target_properties(${TESTNAME} PROPERTIES LINKER_LANGUAGE C)
    target_link_libraries(${TESTNAME} graphite2)

    add_test(NAME ${TESTNAME} COMMAND $<TARGET_FILE:${TESTNAME}> ${ARGN})
    set_tests_properties(${TESTNAME} PROPERTIES TIMEOUT 3)
endmacro()

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    find_package(Freetype)
    if (${FREETYPE_FOUND})
        include_directories(${FREETYPE_INCLUDE_DIRS})
    endif()
endif()

macro(test_freetype TESTNAME SRCFILE)
    if (${FREETYPE_FOUND})
        add_executable(${TESTNAME} ${SRCFILE})
        set_target_properties(${TESTNAME} PROPERTIES LINKER_LANGUAGE C)
        target_link_libraries(${TESTNAME} graphite2 ${FREETYPE_LIBRARIES})

        add_test(NAME ${TESTNAME} COMMAND $<TARGET_FILE:${TESTNAME}> ${ARGN})
        set_tests_properties(${TESTNAME} PROPERTIES TIMEOUT 3)
    endif()
endmacro()

test_example(simple simple.c ${testing_SOURCE_DIR}/fonts/Padauk.ttf "Hello World!")
test_example(features features.c ${testing_SOURCE_DIR}/fonts/Padauk.ttf)
test_example(clusters cluster.c ${testing_SOURCE_DIR}/fonts/Padauk.ttf "စက္ခုန္ဒြေ")
test_example(linebreak linebreak.c ${testing_SOURCE_DIR}/fonts/charis_r_gr.ttf 120 "This is a long test line that goes on and on and on")
test_freetype(freetype freetype.c ${testing_SOURCE_DIR}/fonts/Padauk.ttf "Hello World!")
