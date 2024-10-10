/* SPDX-License-Identifier: MIT */
/* Copyright 2010, SIL International, All rights reserved. */
#pragma once

#include <stddef.h>
#include <stdint.h>

enum gr_encform {
  gr_utf8 = 1/*sizeof(uint8_t)*/, gr_utf16 = 2/*sizeof(uint16_t)*/, gr_utf32 = 4/*sizeof(uint32_t)*/
};


// Define API function declspec/attributes and how each supported compiler or OS
// allows us to specify them.
#if defined __GNUC__
  #define _gr2_and ,
  #define _gr2_tag_fn(a)        __attribute__((a))
  #define _gr2_deprecated_flag  deprecated
  #define _gr2_export_flag      visibility("default")
  #define _gr2_import_flag      visibility("default")
  #define _gr2_static_flag      visibility("hidden")
#endif

#if defined _WIN32 || defined __CYGWIN__
  #if defined __GNUC__  // These three will be redefined for Windows
    #undef _gr2_export_flag
    #undef _gr2_import_flag
    #undef _gr2_static_flag
  #else  // How MSVC sepcifies function level attributes adn deprecation
    #define _gr2_and
    #define _gr2_tag_fn(a)       __declspec(a)
    #define _gr2_deprecated_flag deprecated
  #endif
  #define _gr2_export_flag     dllexport
  #define _gr2_import_flag     dllimport
  #define _gr2_static_flag
#endif

#if defined GRAPHITE2_STATIC
  #define GR2_API             _gr2_tag_fn(_gr2_static_flag)
  #define GR2_DEPRECATED_API  _gr2_tag_fn(_gr2_deprecated_flag _gr2_and _gr2_static_flag)
#elif defined GRAPHITE2_EXPORTING
  #define GR2_API             _gr2_tag_fn(_gr2_export_flag)
  #define GR2_DEPRECATED_API  _gr2_tag_fn(_gr2_deprecated_flag _gr2_and _gr2_export_flag)
#else
  #define GR2_API             _gr2_tag_fn(_gr2_import_flag)
  #define GR2_DEPRECATED_API  _gr2_tag_fn(_gr2_deprecated_flag _gr2_and _gr2_import_flag)
#endif
