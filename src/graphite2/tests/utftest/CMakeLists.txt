# SPDX-License-Identifier: MIT
# Copyright 2011, SIL International, All rights reserved.
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12 FATAL_ERROR)
project(utftest)
include(Graphite)
include_directories(${graphite2_core_SOURCE_DIR})

if  (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -DUNICODE)
    add_custom_target(${PROJECT_NAME}_copy_dll ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${graphite2_core_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}graphite2${CMAKE_SHARED_LIBRARY_SUFFIX} ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
    add_dependencies(${PROJECT_NAME}_copy_dll graphite2 utftest)
endif()


add_executable(utftest utftest.cpp)
target_link_libraries(utftest graphite2)

add_test(NAME utftest COMMAND $<TARGET_FILE:utftest>)
