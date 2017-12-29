add_subdirectory(api)
add_subdirectory(shaping)
# Not ready yet
if (NOT MSVC)
  add_subdirectory(fuzzing)
endif ()
