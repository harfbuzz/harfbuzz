if (HB_HAVE_GLIB)
  add_executable (hb-fuzzer hb-fuzzer.cc hb-fuzzer.hh main.cc)
  target_link_libraries (hb-fuzzer harfbuzz)
  add_test (hb-fuzzer python run-fuzzer-tests.py)
  set_tests_properties (hb-fuzzer PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    ENVIRONMENT "top_builddir=${PROJECT_BINARY_DIR};testdir=test;fuzzingdir=fuzzing"
  )
endif ()
