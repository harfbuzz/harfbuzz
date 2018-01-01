if (HB_BUILD_UTILS)
  file (READ "${CMAKE_CURRENT_SOURCE_DIR}/Makefile.am" MAKEFILEAM)
  extract_make_variable (TESTS ${MAKEFILEAM})

  foreach (test IN ITEMS ${TESTS})
    add_test (NAME ${test}
      COMMAND python run-tests.py $<TARGET_FILE:hb-shape> "${test}"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endforeach ()
endif ()
