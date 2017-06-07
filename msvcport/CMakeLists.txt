cmake_minimum_required(VERSION 2.8.0)

project(harfbuzzng)

if (NOT WIN32 OR (MINGW OR MSYS OR CYGWIN))
  message(FATAL_ERROR
    "This CMake project is intended only for native WIN32 compilation.  "
    "For any other platforms please use the autotools ./configure script.")
endif (NOT WIN32 OR (MINGW OR MSYS OR CYGWIN))

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
endif (FREETYPE_INCLUDE_DIR)

if (FREETYPE_INCLUDE_DIR AND FREETYPE_LIBRARY)
  set (THIRD_PARTY_LIBS ${THIRD_PARTY_LIBS} ${FREETYPE_LIBRARY})
  add_definitions(-DHAVE_FREETYPE=1 -DHAVE_FT_FACE_GETCHARVARIANTINDEX=1)
endif (FREETYPE_INCLUDE_DIR AND FREETYPE_LIBRARY)


find_program(RAGEL "ragel")

if (RAGEL)
  message(STATUS "ragel found at: ${RAGEL}")
else (RAGEL)
  message(FATAL_ERROR "ragel not found, get it here -- http://www.complang.org/ragel/")
endif (RAGEL)

function (ragel_preproc src_dir src_sans_rl out_sfx)
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${src_sans_rl}${out_sfx}
    COMMAND ${RAGEL} -G2 -o ${CMAKE_CURRENT_BINARY_DIR}/${src_sans_rl}${out_sfx} ${CMAKE_CURRENT_SOURCE_DIR}/${src_dir}/${src_sans_rl}.rl -I ${CMAKE_CURRENT_SOURCE_DIR} ${ARGN}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src_dir}/${src_sans_rl}.rl
    )
  add_custom_target(ragel_${src_sans_rl} DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${src_sans_rl})
endfunction(ragel_preproc)


include_directories(AFTER
  ${PROJECT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/.
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  )

add_definitions(-DHAVE_CONFIG_H)

ragel_preproc(../src hb-buffer-deserialize-json .hh)
ragel_preproc(../src hb-buffer-deserialize-text .hh)
ragel_preproc(../src hb-ot-shape-complex-indic-machine .hh)
ragel_preproc(../src hb-ot-shape-complex-myanmar-machine .hh)
ragel_preproc(../src hb-ot-shape-complex-sea-machine .hh)

set (HB_VERSION_MAJOR 0)
set (HB_VERSION_MINOR 9)
set (HB_VERSION_MICRO 28)
set (HB_VERSION "0.9.28")

set (HB_VERSION_H_IN "${PROJECT_SOURCE_DIR}/../src/hb-version.h.in")
set (HB_VERSION_H "${PROJECT_BINARY_DIR}/hb-version.h")
set_source_files_properties("${HB_VERSION_H}" PROPERTIES GENERATED true)
configure_file("${HB_VERSION_H_IN}" "${HB_VERSION_H}.tmp" @ONLY)
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
  "${HB_VERSION_H}.tmp"
  "${HB_VERSION_H}")
file(REMOVE "${HB_VERSION_H}.tmp")


set(project_sources
  ${project_sources}

  ${CMAKE_CURRENT_BINARY_DIR}/hb-buffer-deserialize-json.hh
  ${CMAKE_CURRENT_BINARY_DIR}/hb-buffer-deserialize-text.hh
  ${CMAKE_CURRENT_BINARY_DIR}/hb-ot-shape-complex-indic-machine.hh
  ${CMAKE_CURRENT_BINARY_DIR}/hb-ot-shape-complex-myanmar-machine.hh
  ${CMAKE_CURRENT_BINARY_DIR}/hb-ot-shape-complex-sea-machine.hh
  )

set(project_headers
  ${HB_VERSION_H}
  ../src/hb.h
  ../src/hb-blob.h
  ../src/hb-buffer.h
  ../src/hb-common.h
  ../src/hb-deprecated.h
  ../src/hb-face.h
  ../src/hb-font.h
  ../src/hb-ft.h
  ../src/hb-ot.h
  ../src/hb-ot-layout.h
  ../src/hb-ot-tag.h
  ../src/hb-set.h
  ../src/hb-shape.h
  ../src/hb-shape-plan.h
  ../src/hb-unicode.h)

set(project_sources
  config.h

  ${project_sources}

  ../src/hb-blob.cc
  ../src/hb-buffer.cc
  ../src/hb-buffer-serialize.cc
  ../src/hb-common.cc
  ../src/hb-face.cc
  ../src/hb-font.cc
  ../src/hb-ft.cc
  ../src/hb-ot-tag.cc
  ../src/hb-set.cc
  ../src/hb-shape.cc
  ../src/hb-shape-plan.cc
  ../src/hb-shaper.cc
  ../src/hb-unicode.cc
  ../src/hb-warning.cc

  ../src/hb-atomic-private.hh
  ../src/hb-buffer-private.hh
  ../src/hb-cache-private.hh
  ../src/hb-face-private.hh
  ../src/hb-font-private.hh
  ../src/hb-mutex-private.hh
  ../src/hb-object-private.hh
  ../src/hb-open-file-private.hh
  ../src/hb-open-type-private.hh
  ../src/hb-ot-head-table.hh
  ../src/hb-ot-hhea-table.hh
  ../src/hb-ot-hmtx-table.hh
  ../src/hb-ot-maxp-table.hh
  ../src/hb-ot-name-table.hh
  ../src/hb-private.hh
  ../src/hb-set-private.hh
  ../src/hb-shape-plan-private.hh
  ../src/hb-shaper-impl-private.hh
  ../src/hb-shaper-list.hh
  ../src/hb-shaper-private.hh
  ../src/hb-unicode-private.hh
  ../src/hb-utf-private.hh

  # Open Type
  ../src/hb-ot-layout.cc
  ../src/hb-ot-map.cc
  ../src/hb-ot-shape.cc
  ../src/hb-ot-shape-complex-arabic.cc
  ../src/hb-ot-shape-complex-default.cc
  ../src/hb-ot-shape-complex-hangul.cc
  ../src/hb-ot-shape-complex-hebrew.cc
  ../src/hb-ot-shape-complex-indic.cc
  ../src/hb-ot-shape-complex-indic-table.cc
  ../src/hb-ot-shape-complex-myanmar.cc
  ../src/hb-ot-shape-complex-sea.cc
  ../src/hb-ot-shape-complex-thai.cc
  ../src/hb-ot-shape-complex-tibetan.cc
  ../src/hb-ot-shape-fallback.cc
  ../src/hb-ot-shape-normalize.cc

  ../src/hb-ot-layout-common-private.hh
  ../src/hb-ot-layout-gdef-table.hh
  ../src/hb-ot-layout-gpos-table.hh
  ../src/hb-ot-layout-gsubgpos-private.hh
  ../src/hb-ot-layout-gsub-table.hh
  ../src/hb-ot-layout-jstf-table.hh
  ../src/hb-ot-layout-private.hh
  ../src/hb-ot-map-private.hh
  ../src/hb-ot-shape-complex-arabic-fallback.hh
  ../src/hb-ot-shape-complex-arabic-table.hh
  ../src/hb-ot-shape-complex-indic-private.hh
  ../src/hb-ot-shape-complex-private.hh
  ../src/hb-ot-shape-fallback-private.hh
  ../src/hb-ot-shape-normalize-private.hh
  ../src/hb-ot-shape-private.hh

  ../src/hb-ot-shape.h

  ${project_headers}
  )

set(THIRD_PARTY_LIBS )

if (APPLE)
  # Apple Advanced Typography
  add_definitions(-DHAVE_CORETEXT)

  set(project_sources
    ${project_sources}
    ../src/hb-coretext.cc
    ../src/hb-coretext.h)

  find_library(APPLICATION_SERVICES_FRAMEWORK ApplicationServices)
  mark_as_advanced(APPLICATION_SERVICES_FRAMEWORK)
  if (APPLICATION_SERVICES_FRAMEWORK)
    set(THIRD_PARTY_LIBS ${THIRD_PARTY_LIBS} ${APPLICATION_SERVICES_FRAMEWORK})
  endif (APPLICATION_SERVICES_FRAMEWORK)
endif (APPLE)

add_library(libharfbuzz STATIC ${project_sources})
target_link_libraries(libharfbuzz ${THIRD_PARTY_LIBS})

install(TARGETS libharfbuzz DESTINATION lib)
install(FILES
  ${project_headers}

  DESTINATION
  include/harfbuzz)
