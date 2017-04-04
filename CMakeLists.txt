cmake_minimum_required(VERSION 2.8.0)
project(harfbuzz)

## Disallow in-source builds
if ("${PROJECT_BINARY_DIR}" STREQUAL "${PROJECT_SOURCE_DIR}")
  message(FATAL_ERROR
    "
In-source builds are not permitted!  Make a separate folder for"
    " building, e.g.,"
    "
  mkdir build; cd build; cmake .."
    "
Before that, remove the files created by this failed run with"
    "
  rm -rf CMakeCache.txt CMakeFiles")
endif ()
##

## HarfBuzz build configurations
option(HB_HAVE_FREETYPE "Use FreeType" OFF)
option(HB_HAVE_UNISCRIBE "Enable Uniscribe shaper on Windows" OFF)
option(HB_HAVE_DIRECWRITE "Enable DirectWrite shaper on Windows" OFF)
option(HB_HAVE_CORETEXT "Enable CoreText shaper on macOS" ON)
option(HB_BUILTIN_UCDN "Use HarfBuzz provided UCDN" ON)

include_directories(AFTER
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}/src
  )

# feel free to discuss these or more with the maintainers
add_definitions(-DHAVE_OT)
add_definitions(-DHB_NO_MT)
add_definitions(-DHB_DISABLE_DEPRECATED)

if (BUILD_SHARED_LIBS)
  add_definitions(-DHAVE_ATEXIT)
endif ()

if (MSVC)
  add_definitions(-wd4244 -wd4267 -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS)
endif ()

if (WIN32 AND NOT MINGW AND BUILD_SHARED_LIBS)
  add_definitions("-DHB_EXTERN=__declspec(dllexport) extern")
endif ()
##

set(IN_HB_DIST FALSE)
if (EXISTS "${PROJECT_SOURCE_DIR}/src/hb-version.h")
  # perhaps we are on dist directory
  set(IN_HB_DIST TRUE)
  set(HB_VERSION_H "${PROJECT_SOURCE_DIR}/src/hb-version.h")
endif ()

if (NOT IN_HB_DIST)
  ## execute ragel tasks
  find_program(RAGEL "ragel")

  if (RAGEL)
    message(STATUS "ragel found at: ${RAGEL}")
  else (RAGEL)
    message(FATAL_ERROR "ragel not found, get it here -- http://www.complang.org/ragel/")
  endif (RAGEL)

  function (ragel_preproc src_dir src_sans_rl out_sfx)
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src/${src_sans_rl}${out_sfx}
      COMMAND ${RAGEL} -G2 -o ${PROJECT_BINARY_DIR}/src/${src_sans_rl}${out_sfx} ${CMAKE_CURRENT_SOURCE_DIR}/${src_dir}/${src_sans_rl}.rl -I ${CMAKE_CURRENT_SOURCE_DIR} ${ARGN}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src_dir}/${src_sans_rl}.rl
      )
    add_custom_target(harfbuzz_${src_sans_rl} DEPENDS ${PROJECT_BINARY_DIR}/src/${src_sans_rl})
  endfunction(ragel_preproc)

  ragel_preproc(src hb-buffer-deserialize-json .hh)
  ragel_preproc(src hb-buffer-deserialize-text .hh)
  ragel_preproc(src hb-ot-shape-complex-indic-machine .hh)
  ragel_preproc(src hb-ot-shape-complex-myanmar-machine .hh)
  ragel_preproc(src hb-ot-shape-complex-use-machine .hh)
  ##

  ## Generate hb-version.h
  file(READ configure.ac CONFIGUREAC)
  string(REGEX MATCH "\\[(([0-9]+)\\.([0-9]+)\\.([0-9]+))\\]" HB_VERSION_MATCH ${CONFIGUREAC})
  set(HB_VERSION ${CMAKE_MATCH_1})
  set(HB_VERSION_MAJOR ${CMAKE_MATCH_2})
  set(HB_VERSION_MINOR ${CMAKE_MATCH_3})
  set(HB_VERSION_MICRO ${CMAKE_MATCH_4})

  set(HB_VERSION_H_IN "${PROJECT_SOURCE_DIR}/src/hb-version.h.in")
  set(HB_VERSION_H "${PROJECT_BINARY_DIR}/src/hb-version.h")
  set_source_files_properties("${HB_VERSION_H}" PROPERTIES GENERATED true)
  configure_file("${HB_VERSION_H_IN}" "${HB_VERSION_H}.tmp" @ONLY)
  execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    "${HB_VERSION_H}.tmp"
    "${HB_VERSION_H}")
  file(REMOVE "${HB_VERSION_H}.tmp")
  ##
endif ()

## Extract variables from src/Makefile.sources
file(READ src/Makefile.sources MAKEFILESOURCES)
function (extract_make_variable variable)
  string(REGEX MATCH "${variable} = ([^$]+)" temp ${MAKEFILESOURCES})
  if (NOT IN_HB_DIST)
    # these should be built first and will be added from a different path
    string(REPLACE "hb-buffer-deserialize-json.hh" "" temp ${temp})
    string(REPLACE "hb-buffer-deserialize-text.hh" "" temp ${temp})
    string(REPLACE "hb-ot-shape-complex-indic-machine.hh" "" temp ${temp})
    string(REPLACE "hb-ot-shape-complex-myanmar-machine.hh" "" temp ${temp})
    string(REPLACE "hb-ot-shape-complex-use-machine.hh" "" temp ${temp})
  endif ()
  string(REGEX REPLACE "\thb" "src/hb" temp ${temp})
  string(REGEX MATCHALL "src/[^ ]+" temp ${temp})
  set(${variable} ${temp} PARENT_SCOPE)
endfunction(extract_make_variable)

extract_make_variable(HB_BASE_sources)
extract_make_variable(HB_BASE_headers)
extract_make_variable(HB_OT_sources)
extract_make_variable(HB_OT_headers)
##

## Define source and headers of projects
if (NOT IN_HB_DIST)
  set(project_sources
    ${project_sources}

    ${PROJECT_BINARY_DIR}/src/hb-buffer-deserialize-json.hh
    ${PROJECT_BINARY_DIR}/src/hb-buffer-deserialize-text.hh
    ${PROJECT_BINARY_DIR}/src/hb-ot-shape-complex-indic-machine.hh
    ${PROJECT_BINARY_DIR}/src/hb-ot-shape-complex-myanmar-machine.hh
    ${PROJECT_BINARY_DIR}/src/hb-ot-shape-complex-use-machine.hh
    )
endif ()

set(project_headers
  ${HB_VERSION_H}

  ${HB_BASE_headers}
  ${HB_OT_headers}
  )

set(project_sources
  ${project_sources}

  ${HB_BASE_sources}
  ${HB_OT_sources}
  )

if (HB_HAVE_FREETYPE)
  set(FREETYPE_DIR "$ENV{FREETYPE_DIR}"
    CACHE PATH "root path for freetype lib/ and include/ folders"
    )
  find_path(FREETYPE_INCLUDE_DIR
    ft2build.h freetype2/freetype/freetype.h
    PATHS ${FREETYPE_DIR}/include
    )

  if (CMAKE_BUILD_TYPE MATCHES Debug)
    find_library(FREETYPE_DEBUG_LIBRARY freetyped)
    set(FREETYPE_ACTUAL_LIBRARY ${FREETYPE_DEBUG_LIBRARY})
  else ()
    find_library(FREETYPE_LIBRARY
      freetype libfreetype
      PATHS ${FREETYPE_DIR}/lib
      DOC "freetype library"
      )
    set(FREETYPE_ACTUAL_LIBRARY ${FREETYPE_LIBRARY})
  endif ()

  if (FREETYPE_INCLUDE_DIR)
    include_directories(AFTER ${FREETYPE_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIR}/freetype2)
  endif ()

  if (FREETYPE_INCLUDE_DIR AND FREETYPE_ACTUAL_LIBRARY)
    set (THIRD_PARTY_LIBS ${THIRD_PARTY_LIBS} ${FREETYPE_ACTUAL_LIBRARY})
    add_definitions(-DHAVE_FREETYPE=1 -DHAVE_FT_FACE_GETCHARVARIANTINDEX=1)
  endif ()

  set(project_headers ${project_headers} src/hb-ft.h)

  set(project_sources ${project_sources} src/hb-ft.cc)
endif ()

if (HB_BUILTIN_UCDN)
  include_directories(src/hb-ucdn)
  add_definitions(-DHAVE_UCDN)

  set(project_headers ${project_headers} src/hb-ucdn/ucdn.h)

  set(project_sources
    ${project_sources}

    src/hb-ucdn.cc
    src/hb-ucdn/ucdn.c
    src/hb-ucdn/unicodedata_db.h)
else ()
  add_definitions(-DHB_NO_UNICODE_FUNCS)
endif ()

if (APPLE AND HB_HAVE_CORETEXT)
  # Apple Advanced Typography
  add_definitions(-DHAVE_CORETEXT)

  set(project_headers ${project_headers} src/hb-coretext.h)

  set(project_sources ${project_sources} src/hb-coretext.cc)

  find_library(APPLICATION_SERVICES_FRAMEWORK ApplicationServices)
  mark_as_advanced(APPLICATION_SERVICES_FRAMEWORK)
  if (APPLICATION_SERVICES_FRAMEWORK)
    set(THIRD_PARTY_LIBS ${THIRD_PARTY_LIBS} ${APPLICATION_SERVICES_FRAMEWORK})
  endif (APPLICATION_SERVICES_FRAMEWORK)
endif ()

if (WIN32 AND HB_HAVE_UNISCRIBE)
  add_definitions(-DHAVE_UNISCRIBE)

  set(project_headers ${project_headers} src/hb-uniscribe.h)

  set(project_sources ${project_sources} src/hb-uniscribe.cc)

  set(THIRD_PARTY_LIBS ${THIRD_PARTY_LIBS} usp10 gdi32 rpcrt4)
endif ()

if (WIN32 AND HB_HAVE_DIRECTWRITE)
  add_definitions(-DHAVE_DIRECTWRITE)

  set(project_headers ${project_headers} src/hb-directwrite.h)

  set(project_sources ${project_sources} src/hb-directwrite.cc)

  set(THIRD_PARTY_LIBS ${THIRD_PARTY_LIBS} dwrite rpcrt4)
endif ()

set(project_sources ${project_sources} ${project_headers})
##

add_library(harfbuzz ${project_sources})
target_link_libraries(harfbuzz ${THIRD_PARTY_LIBS})

## Install
if (NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL)
  install(FILES ${project_headers} DESTINATION include/harfbuzz)
endif ()

if (NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL)
  install(TARGETS harfbuzz
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    )
endif ()
##