if (HB_BUILD_UTILS)
  file (READ "${CMAKE_CURRENT_SOURCE_DIR}/data/Makefile.sources" SOURCES)
  extract_make_variable (TESTS ${SOURCES})
  foreach (test IN ITEMS ${TESTS})
    add_test (NAME ${test}
      COMMAND python run-tests.py $<TARGET_FILE:hb-subset> "data/${test}"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endforeach ()
endif ()
