cmake_minimum_required(VERSION 2.8.0)
project(harfbuzz)

## HarfBuzz build configurations
option(HB_INSTALL "Generate installation target" ON)
option(HB_HAVE_FREETYPE "Use FreeType" OFF)
option(HB_HAVE_UNISCRIBE "Uniscribe shaper" OFF)
option(HB_HAVE_DIRECWRITE "DirectWrite shaper" OFF)
option(HB_HAVE_CORETEXT "Enable CoreText shaper on Mac" ON)
option(HB_BUILTIN_UCDN "Use HarfBuzz provided UCDN" ON)

include_directories(AFTER
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}
  )

add_definitions(-DHAVE_OT)
add_definitions(-DHAVE_ATEXIT)
add_definitions(-DHB_NO_MT)
add_definitions(-DHB_DISABLE_DEPRECATED)
##

## execute ragel tasks
find_program(RAGEL "ragel")

if (RAGEL)
  message(STATUS "ragel found at: ${RAGEL}")
else (RAGEL)
  message(FATAL_ERROR "ragel not found, get it here -- http://www.complang.org/ragel/")
endif (RAGEL)

function (ragel_preproc src_dir src_sans_rl out_sfx)
  add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/${src_sans_rl}${out_sfx}
    COMMAND ${RAGEL} -G2 -o ${PROJECT_BINARY_DIR}/${src_sans_rl}${out_sfx} ${CMAKE_CURRENT_SOURCE_DIR}/${src_dir}/${src_sans_rl}.rl -I ${CMAKE_CURRENT_SOURCE_DIR} ${ARGN}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src_dir}/${src_sans_rl}.rl
    )
  add_custom_target(harfbuzz_${src_sans_rl} DEPENDS ${PROJECT_BINARY_DIR}/${src_sans_rl})
endfunction(ragel_preproc)

ragel_preproc(src hb-buffer-deserialize-json .hh)
ragel_preproc(src hb-buffer-deserialize-text .hh)
ragel_preproc(src hb-ot-shape-complex-indic-machine .hh)
ragel_preproc(src hb-ot-shape-complex-myanmar-machine .hh)
ragel_preproc(src hb-ot-shape-complex-use-machine .hh)
##

## Make hb-version.h
file(READ configure.ac CONFIGUREAC)
string(REGEX MATCH "\\[(([0-9]+)\\.([0-9]+)\\.([0-9]+))\\]" HB_VERSION_MATCH ${CONFIGUREAC})
set(HB_VERSION ${CMAKE_MATCH_1})
set(HB_VERSION_MAJOR ${CMAKE_MATCH_2})
set(HB_VERSION_MINOR ${CMAKE_MATCH_3})
set(HB_VERSION_MICRO ${CMAKE_MATCH_4})

set(HB_VERSION_H_IN "${PROJECT_SOURCE_DIR}/src/hb-version.h.in")
set(HB_VERSION_H "${PROJECT_BINARY_DIR}/hb-version.h")
set_source_files_properties("${HB_VERSION_H}" PROPERTIES GENERATED true)
configure_file("${HB_VERSION_H_IN}" "${HB_VERSION_H}.tmp" @ONLY)
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
  "${HB_VERSION_H}.tmp"
  "${HB_VERSION_H}")
file(REMOVE "${HB_VERSION_H}.tmp")
##

## Define source and headers of projects
set(project_sources
  ${project_sources}

  ${PROJECT_BINARY_DIR}/hb-buffer-deserialize-json.hh
  ${PROJECT_BINARY_DIR}/hb-buffer-deserialize-text.hh
  ${PROJECT_BINARY_DIR}/hb-ot-shape-complex-indic-machine.hh
  ${PROJECT_BINARY_DIR}/hb-ot-shape-complex-myanmar-machine.hh
  ${PROJECT_BINARY_DIR}/hb-ot-shape-complex-use-machine.hh
  )

set(project_headers
  ${HB_VERSION_H}

  src/hb.h
  src/hb-blob.h
  src/hb-buffer.h
  src/hb-common.h
  src/hb-deprecated.h
  src/hb-face.h
  src/hb-font.h
  src/hb-ot.h
  src/hb-ot-layout.h
  src/hb-ot-tag.h
  src/hb-set.h
  src/hb-shape.h
  src/hb-shape-plan.h
  src/hb-unicode.h
  )

set(project_sources
  ${project_sources}

  src/hb-blob.cc
  src/hb-buffer.cc
  src/hb-buffer-serialize.cc
  src/hb-common.cc
  src/hb-face.cc
  src/hb-font.cc
  src/hb-ot-font.cc
  src/hb-ot-tag.cc
  src/hb-set.cc
  src/hb-shape.cc
  src/hb-shape-plan.cc
  src/hb-shaper.cc
  src/hb-unicode.cc
  src/hb-warning.cc

  src/hb-atomic-private.hh
  src/hb-buffer-private.hh
  src/hb-cache-private.hh
  src/hb-face-private.hh
  src/hb-font-private.hh
  src/hb-mutex-private.hh
  src/hb-object-private.hh
  src/hb-open-file-private.hh
  src/hb-open-type-private.hh
  src/hb-ot-head-table.hh
  src/hb-ot-hhea-table.hh
  src/hb-ot-hmtx-table.hh
  src/hb-ot-maxp-table.hh
  src/hb-ot-name-table.hh
  src/hb-ot-os2-table.hh
  src/hb-private.hh
  src/hb-set-private.hh
  src/hb-shape-plan-private.hh
  src/hb-shaper-impl-private.hh
  src/hb-shaper-list.hh
  src/hb-shaper-private.hh
  src/hb-unicode-private.hh
  src/hb-utf-private.hh

  # Open Type
  src/hb-ot-layout.cc
  src/hb-ot-map.cc
  src/hb-ot-shape.cc
  src/hb-ot-shape-complex-arabic.cc
  src/hb-ot-shape-complex-default.cc
  src/hb-ot-shape-complex-hangul.cc
  src/hb-ot-shape-complex-hebrew.cc
  src/hb-ot-shape-complex-indic.cc
  src/hb-ot-shape-complex-indic-table.cc
  src/hb-ot-shape-complex-myanmar.cc
  src/hb-ot-shape-complex-use.cc
  src/hb-ot-shape-complex-use-table.cc
  src/hb-ot-shape-complex-thai.cc
  src/hb-ot-shape-complex-tibetan.cc
  src/hb-ot-shape-fallback.cc
  src/hb-ot-shape-normalize.cc

  src/hb-ot-layout-common-private.hh
  src/hb-ot-layout-gdef-table.hh
  src/hb-ot-layout-gpos-table.hh
  src/hb-ot-layout-gsubgpos-private.hh
  src/hb-ot-layout-gsub-table.hh
  src/hb-ot-layout-jstf-table.hh
  src/hb-ot-layout-private.hh
  src/hb-ot-map-private.hh
  src/hb-ot-shape-complex-arabic-fallback.hh
  src/hb-ot-shape-complex-arabic-table.hh
  src/hb-ot-shape-complex-indic-private.hh
  src/hb-ot-shape-complex-private.hh
  src/hb-ot-shape-fallback-private.hh
  src/hb-ot-shape-normalize-private.hh
  src/hb-ot-shape-private.hh

  src/hb-ot-var.cc
  src/hb-ot-var-avar-table.hh
  src/hb-ot-var-fvar-table.hh
  src/hb-ot-var-hvar-table.hh
  src/hb-ot-var-mvar-table.hh

  src/hb-ot-shape.h
  )

if (HB_HAVE_FREETYPE)
  set(FREETYPE_DIR "$ENV{FREETYPE_DIR}"
    CACHE PATH "root path for freetype lib/ and include/ folders"
    )
  find_path(FREETYPE_INCLUDE_DIR
    ft2build.h freetype2/freetype/freetype.h
    PATHS ${FREETYPE_DIR}/include
    )
  find_library(FREETYPE_LIBRARY
    freetype libfreetype
    PATHS ${FREETYPE_DIR}/lib
    DOC "freetype library"
    )
  if (FREETYPE_INCLUDE_DIR)
    include_directories(AFTER ${FREETYPE_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIR}/freetype2)
  endif ()

  if (FREETYPE_INCLUDE_DIR AND FREETYPE_LIBRARY)
    set (THIRD_PARTY_LIBS ${THIRD_PARTY_LIBS} ${FREETYPE_LIBRARY})
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

add_library(harfbuzz STATIC ${project_sources})
target_link_libraries(harfbuzz ${THIRD_PARTY_LIBS})

if (HB_INSTALL)
  install(TARGETS harfbuzz DESTINATION lib)
  install(FILES
    ${project_headers}

    DESTINATION
    include/harfbuzz)
endif()
