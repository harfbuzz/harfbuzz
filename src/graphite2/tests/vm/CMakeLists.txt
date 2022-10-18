# SPDX-License-Identifier: MIT
# Copyright 2010, SIL International, All rights reserved.
project(vm-testing)
include(Graphite)

enable_testing()

include_directories(${graphite2_core_SOURCE_DIR})

set(S ${graphite2_core_SOURCE_DIR})

# I build the vm code here since it needs to be built for both code threading
# models rather than linking against the library from source.
add_library(vm-test-common STATIC
    basic_test.cpp)
target_link_libraries(vm-test-common graphite2 graphite2-file graphite2-base)
add_definitions(-DGRAPHITE2_NTRACING)

if  (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -DUNICODE)
    add_custom_target(${PROJECT_NAME}_copy_dll ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${graphite2_core_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}graphite2${CMAKE_SHARED_LIBRARY_SUFFIX} ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
    add_dependencies(${PROJECT_NAME}_copy_dll graphite2 vm-test-common)
endif()

add_executable(vm-test-call ${S}/call_machine.cpp)
target_link_libraries(vm-test-call vm-test-common)

if  (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	add_executable(vm-test-direct ${S}/direct_machine.cpp)
	target_link_libraries(vm-test-direct vm-test-common)
endif()

if  (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	add_definitions(-fno-rtti -fno-exceptions)
	if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
		add_definitions(-DNDEBUG -fomit-frame-pointer)
	endif()
endif()

add_test(vm-test-call-threading vm-test-call ${testing_SOURCE_DIR}/fonts/small.ttf 1)
set_tests_properties(vm-test-call-threading PROPERTIES
        PASS_REGULAR_EXPRESSION "simple program size:    14 bytes.*result of program: 42"
        FAIL_REGULAR_EXPRESSION "program terminated early;stack not empty")

if  (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	add_test(vm-test-direct-threading vm-test-direct ${testing_SOURCE_DIR}/fonts/small.ttf 1)
	set_tests_properties(vm-test-direct-threading PROPERTIES
			PASS_REGULAR_EXPRESSION "simple program size:    14 bytes.*result of program: 42"
			FAIL_REGULAR_EXPRESSION "program terminated early;stack not empty")
endif ()
