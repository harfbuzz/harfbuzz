if (HB_BUILD_UTILS)
  file (READ "${CMAKE_CURRENT_SOURCE_DIR}/Makefile.am" MAKEFILEAM)
  extract_make_variable (TESTS ${MAKEFILEAM})
  foreach (test IN ITEMS ${TESTS})
    add_test (${test} python run-tests.py "${test}")
    set_tests_properties (${test} PROPERTIES
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      ENVIRONMENT "top_builddir=${PROJECT_BINARY_DIR};utildir=."
    )
  endforeach ()
endif ()
