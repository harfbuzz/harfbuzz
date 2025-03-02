# SPDX-License-Identifier: MIT
# Copyright 2011, SIL International, All rights reserved.
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12 FATAL_ERROR)
project(jsontest)
include(Graphite)
include_directories(${graphite2_core_SOURCE_DIR})

if  (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -DUNICODE)
endif()


add_executable(jsontest jsontest.cpp ${graphite2_core_SOURCE_DIR}/json.cpp)

add_test(NAME jsontest COMMAND $<TARGET_FILE:jsontest> jsontest.log)
add_test(NAME jsontestOutput COMMAND ${CMAKE_COMMAND} -E compare_files ${PROJECT_BINARY_DIR}/jsontest.log ${testing_SOURCE_DIR}/standards/jsontest.log)
set_tests_properties(jsontestOutput PROPERTIES DEPENDS jsontest)
