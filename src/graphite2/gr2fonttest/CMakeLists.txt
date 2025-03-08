# SPDX-License-Identifier: MIT
# Copyright 2010, SIL International, All rights reserved.
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12 FATAL_ERROR)

project(gr2fonttest)

enable_testing()

if  (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -DUNICODE)
endif()

add_executable(gr2fonttest gr2FontTest.cpp UtfCodec.cpp)
target_link_libraries(gr2fonttest graphite2)

if (BUILD_SHARED_LIBS)
    # copy the DLL so that gr2fonttest can find it
    add_custom_target(${PROJECT_NAME}_copy_dll ALL
	    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${graphite2_core_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}graphite2${CMAKE_SHARED_LIBRARY_SUFFIX} ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
    add_dependencies(${PROJECT_NAME}_copy_dll graphite2 gr2fonttest)
endif()

install(TARGETS gr2fonttest EXPORT gr2fonttest RUNTIME DESTINATION bin)
