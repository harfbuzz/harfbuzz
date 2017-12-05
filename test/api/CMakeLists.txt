macro (_add_tests)
  foreach(test_name ${ARGV})
    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.c)
      add_executable (${test_name} ${test_name}.c)
    elseif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.cc)
      add_executable (${test_name} ${test_name}.cc)
    else ()
      message(FATAL_ERROR "No source file found for test ${test_name}")
    endif ()
    target_link_libraries (${test_name} harfbuzz)
    add_test (${test_name} ${test_name})
  endforeach ()
  set_tests_properties (${ARGV} PROPERTIES ENVIRONMENT
    "G_TEST_SRCDIR=${CMAKE_CURRENT_SOURCE_DIR};G_TEST_BUILDDIR=${CMAKE_CURRENT_BINARY_DIR}"
    )
endmacro ()

if (HB_HAVE_GLIB)
  _add_tests (
    test-blob
    test-buffer
    test-common
    test-font
    test-object
    test-set
    test-shape
    test-unicode
    test-version
    test-ot-tag
    test-c
    test-cplusplus
    )

  if (HB_HAVE_FREETYPE)
    _add_tests (
      test-ot-math
      )
  endif (HB_HAVE_FREETYPE)
endif (HB_HAVE_GLIB)
