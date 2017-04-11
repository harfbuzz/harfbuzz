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
option(HB_HAVE_GLIB "Use glib unicode functions" OFF)

include_directories(AFTER
  ${PROJECT_SOURCE_DIR}/src
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

## Extract variables from src/Makefile.sources
file(READ ${PROJECT_SOURCE_DIR}/src/Makefile.sources MAKEFILESOURCES)
function (extract_make_variable variable directory)
  string(REGEX MATCH "${variable} = ([^$]+)" temp ${MAKEFILESOURCES})
  string(REGEX REPLACE "\thb" "${directory}/hb" temp ${temp})
  string(REGEX MATCHALL "src/[^ ]+" temp ${temp})
  set(${variable} ${temp} PARENT_SCOPE)
endfunction ()

extract_make_variable(HB_BASE_sources "${PROJECT_SOURCE_DIR}/src")
extract_make_variable(HB_BASE_headers "${PROJECT_SOURCE_DIR}/src")
extract_make_variable(HB_OT_sources "${PROJECT_SOURCE_DIR}/src")
extract_make_variable(HB_OT_headers "${PROJECT_SOURCE_DIR}/src")
if (IN_HB_DIST)
  extract_make_variable(HB_BASE_RAGEL_GENERATED_sources "${PROJECT_SOURCE_DIR}/src")
  extract_make_variable(HB_OT_RAGEL_GENERATED_sources "${PROJECT_SOURCE_DIR}/src")
else ()
  extract_make_variable(HB_BASE_RAGEL_GENERATED_sources "${PROJECT_BINARY_DIR}/src")
  extract_make_variable(HB_OT_RAGEL_GENERATED_sources "${PROJECT_BINARY_DIR}/src")
endif ()
##

if (NOT IN_HB_DIST)
  ## execute ragel tasks
  find_program(RAGEL "ragel")

  if (RAGEL)
    message(STATUS "ragel found at: ${RAGEL}")
  else ()
    message(FATAL_ERROR "ragel not found, get it here -- http://www.complang.org/ragel/")
  endif ()

  foreach (ragel_output IN ITEMS ${HB_BASE_RAGEL_GENERATED_sources} ${HB_OT_RAGEL_GENERATED_sources})
    string(REGEX MATCH "([^/]+)\\.hh" temp ${ragel_output})
    set(target_name ${CMAKE_MATCH_1})
    add_custom_command(OUTPUT ${ragel_output}
      COMMAND ${RAGEL} -G2 -o ${ragel_output} ${PROJECT_SOURCE_DIR}/src/${target_name}.rl -I ${PROJECT_SOURCE_DIR} ${ARGN}
      DEPENDS ${PROJECT_SOURCE_DIR}/src/${target_name}.rl
      )
    add_custom_target(harfbuzz_${target_name} DEPENDS ${PROJECT_BINARY_DIR}/src/${target_name})
  endforeach ()
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

## Define source and headers of projects
set(project_sources
  ${HB_BASE_sources}
  ${HB_BASE_RAGEL_GENERATED_sources}

  ${HB_OT_sources}
  ${HB_OT_RAGEL_GENERATED_sources}
  )

set(project_headers
  ${HB_VERSION_H}

  ${HB_BASE_headers}
  ${HB_OT_headers}
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
    set(THIRD_PARTY_LIBS ${THIRD_PARTY_LIBS} ${FREETYPE_ACTUAL_LIBRARY})
    add_definitions(-DHAVE_FREETYPE=1 -DHAVE_FT_FACE_GETCHARVARIANTINDEX=1)
  endif ()

  set(project_sources ${project_sources} ${PROJECT_SOURCE_DIR}/src/hb-ft.cc)
  set(project_headers ${project_headers} ${PROJECT_SOURCE_DIR}/src/hb-ft.h)
endif ()

if (HB_BUILTIN_UCDN)
  include_directories(src/hb-ucdn)
  add_definitions(-DHAVE_UCDN)

  set(project_headers ${project_headers} ${PROJECT_SOURCE_DIR}/src/hb-ucdn/ucdn.h)

  set(project_sources
    ${project_sources}

    ${PROJECT_SOURCE_DIR}/src/hb-ucdn.cc
    ${PROJECT_SOURCE_DIR}/src/hb-ucdn/ucdn.c
    ${PROJECT_SOURCE_DIR}/src/hb-ucdn/unicodedata_db.h)
endif ()

if (HB_HAVE_GLIB)
  add_definitions(-DHAVE_GLIB)

  # https://github.com/WebKit/webkit/blob/master/Source/cmake/FindGLIB.cmake
  find_package(PkgConfig)
  pkg_check_modules(PC_GLIB QUIET glib-2.0)

  find_library(GLIB_LIBRARIES NAMES glib-2.0 HINTS ${PC_GLIB_LIBDIR} ${PC_GLIB_LIBRARY_DIRS})
  find_path(GLIBCONFIG_INCLUDE_DIR NAMES glibconfig.h HINTS ${PC_LIBDIR} ${PC_LIBRARY_DIRS} ${PC_GLIB_INCLUDEDIR} ${PC_GLIB_INCLUDE_DIRS} PATH_SUFFIXES glib-2.0/include)
  find_path(GLIB_INCLUDE_DIR NAMES glib.h HINTS ${PC_GLIB_INCLUDEDIR} ${PC_GLIB_INCLUDE_DIRS} PATH_SUFFIXES glib-2.0)

  include_directories(${GLIBCONFIG_INCLUDE_DIR} ${GLIB_INCLUDE_DIR})

  set(project_sources ${project_sources} ${PROJECT_SOURCE_DIR}/src/hb-glib.cc)
  set(project_headers ${project_headers} ${PROJECT_SOURCE_DIR}/src/hb-glib.h)

  set(THIRD_PARTY_LIBS ${THIRD_PARTY_LIBS} ${GLIB_LIBRARIES})
endif ()

if (APPLE AND HB_HAVE_CORETEXT)
  # Apple Advanced Typography
  add_definitions(-DHAVE_CORETEXT)

  set(project_sources ${project_sources} ${PROJECT_SOURCE_DIR}/src/hb-coretext.cc)
  set(project_headers ${project_headers} ${PROJECT_SOURCE_DIR}/src/hb-coretext.h)

  find_library(APPLICATION_SERVICES_FRAMEWORK ApplicationServices)
  mark_as_advanced(APPLICATION_SERVICES_FRAMEWORK)
  if (APPLICATION_SERVICES_FRAMEWORK)
    set(THIRD_PARTY_LIBS ${THIRD_PARTY_LIBS} ${APPLICATION_SERVICES_FRAMEWORK})
  endif (APPLICATION_SERVICES_FRAMEWORK)
endif ()

if (WIN32 AND HB_HAVE_UNISCRIBE)
  add_definitions(-DHAVE_UNISCRIBE)

  set(project_sources ${project_sources} ${PROJECT_SOURCE_DIR}/src/hb-uniscribe.cc)
  set(project_headers ${project_headers} ${PROJECT_SOURCE_DIR}/src/hb-uniscribe.h)

  set(THIRD_PARTY_LIBS ${THIRD_PARTY_LIBS} usp10 gdi32 rpcrt4)
endif ()

if (WIN32 AND HB_HAVE_DIRECTWRITE)
  add_definitions(-DHAVE_DIRECTWRITE)

  set(project_sources ${project_sources} ${PROJECT_SOURCE_DIR}/src/hb-directwrite.cc)
  set(project_headers ${project_headers} ${PROJECT_SOURCE_DIR}/src/hb-directwrite.h)

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
