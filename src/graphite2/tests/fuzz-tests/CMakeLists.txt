# SPDX-License-Identifier: MIT
# Copyright 2012, SIL International, All rights reserved.
project(fuzztesting)

function(trimtail VAR RESULT)
    STRING(REGEX MATCH "[^/]+$" trimtail_result ${${VAR}})
    set(${RESULT} ${trimtail_result} PARENT_SCOPE)
    STRING(REGEX REPLACE "^(.*)/[^/]+$" "\\1" trimtail_remainder ${${VAR}})
    set(${VAR} ${trimtail_remainder} PARENT_SCOPE)
endfunction()

function(hasr VAR RESULT)
    STRING(REGEX MATCH "-r(\\.|$)" hasr_result ${${VAR}})
    if (hasr_result)
        set(${RESULT} "-r" PARENT_SCOPE)
    else ()
        set(${RESULT} "" PARENT_SCOPE)
    endif ()
endfunction()

 add_custom_target(fuzztest)
 file(GLOB_RECURSE tfiles RELATIVE ${PROJECT_SOURCE_DIR} *.fuzz)
 set(lastfontname "")
 set(excludednames "libfuzz-corpus")
 if (NOT GRAPHITE2_SANITIZERS MATCHES "address")
     set(noasan -t 20 --memory=8000 --valgrind)
 endif ()

 foreach (tfile IN LISTS tfiles)
     set(tpath ${tfile})
     trimtail(tpath tname)
     trimtail(tpath textname)
     trimtail(tpath fname)
     if (NOT excludednames MATCHES ${fname})
         hasr(tname dirargs)
         if (NOT lastfontname EQUAL fname)
             add_custom_command(TARGET fuzztest PRE_BUILD COMMAND echo ${fname})
             set(lastfontname ${fname})
         endif()
         add_custom_command(TARGET fuzztest PRE_BUILD COMMAND ${PROJECT_SOURCE_DIR}/../fuzztest ARGS -l fuzzregress-${fname}-${textname}-${tname}.log -f ${PROJECT_SOURCE_DIR}/../fonts/${fname}.ttf -q ${noasan} --include=required,graphite --exclude==fontdir,opentype,volt,advtypo,post -V -k --input=${PROJECT_SOURCE_DIR}/${fname}/${textname}/${tname} -- ../comparerenderer/comparerenderer -q ${dirargs} -s 12 -n -f {} -t ${PROJECT_SOURCE_DIR}/../texts/${textname}.txt)
         add_custom_command(TARGET fuzztest POST_BUILD COMMAND test -s fuzzregress-${fname}-${textname}-${tname}.log && echo test: ${fname} ${textname} ${tname} failed. || true)
         add_custom_command(TARGET fuzztest PRE_BUILD COMMAND ${PROJECT_SOURCE_DIR}/../fuzztest ARGS -l fuzzregress-demand-${fname}-${textname}-${tname}.log -f ${PROJECT_SOURCE_DIR}/../fonts/${fname}.ttf -q ${noasan} --include=required,graphite --exclude==fontdir,opentype,volt,advtypo,post -V -k --input=${PROJECT_SOURCE_DIR}/${fname}/${textname}/${tname} -- ../comparerenderer/comparerenderer --demand -q ${dirargs} -s 12 -n -f {} -t ${PROJECT_SOURCE_DIR}/../texts/${textname}.txt)
         add_custom_command(TARGET fuzztest POST_BUILD COMMAND test -s fuzzregress-demand-${fname}-${textname}-${tname}.log && echo demand test: ${fname} ${textname} ${tname} failed. || true)
     endif ()
 endforeach()

if (GRAPHITE2_SANITIZERS MATCHES "fuzzer")
  add_executable(gr-fuzzer-segment gr-fuzzer-segment.cpp)
  set_target_properties(gr-fuzzer-segment PROPERTIES
    LINKER_LANGUAGE C
    COMPILE_FLAGS  "-fsanitize=address,${GRAPHITE2_SANITIZERS} -g -O1 -Wall -Wextra -Wno-unknown-pragmas -Wendif-labels -Wctor-dtor-privacy -Wnon-virtual-dtor"
    LINK_FLAGS "-fsanitize=address,${GRAPHITE2_SANITIZERS}")
  target_link_libraries(gr-fuzzer-segment graphite2 graphite2-base ${GRAPHITE2_FUZZING_ENGINE})

  add_executable(gr-fuzzer-font gr-fuzzer-font.cpp)
  set_target_properties(gr-fuzzer-font PROPERTIES
    LINKER_LANGUAGE C
    COMPILE_FLAGS  "-fsanitize=address,${GRAPHITE2_SANITIZERS} -g -O1 -Wall -Wextra -Wno-unknown-pragmas -Wendif-labels -Wctor-dtor-privacy -Wnon-virtual-dtor "
    LINK_FLAGS "-fsanitize=address,${GRAPHITE2_SANITIZERS}")
  target_link_libraries(gr-fuzzer-font graphite2 graphite2-base ${GRAPHITE2_FUZZING_ENGINE})
endif ()

#add_test(NAME graphite-fuzzer COMMAND $<TARGET_FILE:${TESTNAME}> ${ARGN})
#set_tests_properties(graphite-fuzzer PROPERTIES TIMEOUT 30)
