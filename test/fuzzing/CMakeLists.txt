if (HB_HAVE_GLIB)
  file (READ "${CMAKE_CURRENT_SOURCE_DIR}/Makefile.am" MAKEFILEAM)
  extract_make_variable (hb_fuzzer_SOURCES ${MAKEFILEAM})
  add_executable (hb-fuzzer ${hb_fuzzer_SOURCES})
  target_link_libraries (hb-fuzzer harfbuzz)
  add_test (NAME hb-fuzzer
    COMMAND python run-fuzzer-tests.py $<TARGET_FILE:hb-fuzzer>
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif ()
