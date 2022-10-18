# SPDX-License-Identifier: MIT
# Copyright 2010, SIL International, All rights reserved.
project(comparerenderer)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(RT_LDFLAGS "rt")
endif()

include_directories(../common ${PROJECT_SOURCE_DIR}/../../src)

file(GLOB PRIVATE_HEADERS *.h)
set_source_files_properties(${PRIVATE_HEADERS} PROPERTIES HEADER_FILE_ONLY true)

add_executable(comparerenderer
    CompareRenderer.cpp
    ${PRIVATE_HEADERS})

if  (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set_target_properties(comparerenderer PROPERTIES
    COMPILE_FLAGS   "-std=c++11")
endif()

# rt is for timing
target_link_libraries(comparerenderer graphite2-base graphite2 ${FREETYPE_LIBRARIES} ${GLIB_LD_FLAGS} ${HB_LDFLAGS} ${HB1_LDFLAGS} ${GR_LDFLAGS} ${ICU_LDFLAGS} ${RT_LDFLAGS})

if  (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
    add_custom_target(${PROJECT_NAME}_copy_dll ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${graphite2_core_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}graphite2${CMAKE_SHARED_LIBRARY_SUFFIX} ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
    add_dependencies(${PROJECT_NAME}_copy_dll graphite2 iconv comparerenderer)
endif()

macro(comparetest TESTNAME FONTFILE TEXTFILE)
    if (${GR_FOUND})
    # comparision tests between old and new graphite
        add_test(NAME ${TESTNAME}_comparison COMMAND $<TARGET_FILE:comparerenderer>
        -t ${testing_SOURCE_DIR}/texts/${TEXTFILE} -f ${testing_SOURCE_DIR}/fonts/${FONTFILE} -s 12 -v -l ${PROJECT_BINARY_DIR}/${TESTNAME}.log -n --tolerance 0.0001 ${ARGN})
        set_tests_properties(${TESTNAME}_comparison PROPERTIES TIMEOUT 3)
        add_test(NAME ${TESTNAME}cmpOutput COMMAND ${CMAKE_COMMAND} -E compare_files ${PROJECT_BINARY_DIR}/${TESTNAME}.log ${PROJECT_SOURCE_DIR}/../standards/${TESTNAME}${PLATFORM_TEST_SUFFIX}.log)
        set_tests_properties(${TESTNAME}cmpOutput PROPERTIES DEPENDS ${TESTNAME})
    endif()
endmacro()

comparetest(charis charis_r_gr.ttf udhr_eng.txt)
comparetest(padauk Padauk.ttf my_HeadwordSyllables.txt)
comparetest(padauk_ulon Padauk.ttf my_HeadwordSyllables.txt --features ulon=1)
comparetest(charis_yor charis_r_gr.ttf udhr_yor.txt)
comparetest(piglatin PigLatinBenchmark_v3.ttf udhr_eng.txt)
# silgraphite and gr2 bidi algorithms don't quite agree enough
# comparetest(scheherazade_arb Scheherazadegr.ttf udhr_arb.txt --rtl)
comparetest(annapurna_nep Annapurnarc2.ttf udhr_nep.txt)

# comparerenderer can also be used for font testing
# install(TARGETS comparerenderer EXPORT comparerenderer RUNTIME DESTINATION bin)
