if (HB_CHECK)
  file (READ "${CMAKE_CURRENT_SOURCE_DIR}/Makefile.am" MAKEFILEAM)
  extract_make_variable (hb_fuzzer_SOURCES ${MAKEFILEAM})

  # TODO: enable these two
  #extract_make_variable (FUZZING_CPPFLAGS ${MAKEFILEAM}) # extracting regex fail
  #add_executable (hb-fuzzer # it should be run only after ragel execution
  #  ${project_sources} ${project_extra_sources} ${project_headers}
  #  ${hb_fuzzer_SOURCES})

  add_executable (hb-fuzzer ${hb_fuzzer_SOURCES})
  target_link_libraries (hb-fuzzer harfbuzz)

  target_compile_definitions(hb-fuzzer PUBLIC ${FUZZING_CPPFLAGS})
  add_test (NAME hb-fuzzer
    COMMAND python run-fuzzer-tests.py $<TARGET_FILE:hb-fuzzer>
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif ()
