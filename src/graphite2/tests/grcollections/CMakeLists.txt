# SPDX-License-Identifier: MIT
# Copyright 2010, SIL International, All rights reserved.
project(grlisttest)

include_directories(${graphite2_core_SOURCE_DIR})

set(S ${graphite2_core_SOURCE_DIR})

add_executable(vectortest vectortest.cpp)
add_executable(listtest listtest.cpp)
add_test(NAME vector COMMAND $<TARGET_FILE:vectortest>)
add_test(NAME list COMMAND $<TARGET_FILE:listtest>)

# add_executable(intervalsettest intervalsettest.cpp)
# target_link_libraries(intervalsettest graphite2 graphite2-base)
# add_test(NAME intervalset COMMAND $<TARGET_FILE:intervalsettest>)
