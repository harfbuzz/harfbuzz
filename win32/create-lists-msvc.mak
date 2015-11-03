# Convert the source listing to object (.obj) listing in
# another NMake Makefile module, include it, and clean it up.
# This is a "fact-of-life" regarding NMake Makefiles...
# This file does not need to be changed unless one is maintaining the NMake Makefiles

# For those wanting to add things here:
# To add a list, do the following:
# # $(description_of_list)
# if [call create-lists.bat header $(makefile_snippet_file) $(variable_name)]
# endif
#
# if [call create-lists.bat file $(makefile_snippet_file) $(file_name)]
# endif
#
# if [call create-lists.bat footer $(makefile_snippet_file)]
# endif
# ... (repeat the if [call ...] lines in the above order if needed)
# !include $(makefile_snippet_file)
#
# (add the following after checking the entries in $(makefile_snippet_file) is correct)
# (the batch script appends to $(makefile_snippet_file), you will need to clear the file unless the following line is added)
#!if [del /f /q $(makefile_snippet_file)]
#!endif

# In order to obtain the .obj filename that is needed for NMake Makefiles to build DLLs/static LIBs or EXEs, do the following
# instead when doing 'if [call create-lists.bat file $(makefile_snippet_file) $(file_name)]'
# (repeat if there are multiple $(srcext)'s in $(source_list), ignore any headers):
# !if [for %c in ($(source_list)) do @if "%~xc" == ".$(srcext)" @call create-lists.bat file $(makefile_snippet_file) $(intdir)\%~nc.obj]
#
# $(intdir)\%~nc.obj needs to correspond to the rules added in build-rules-msvc.mak
# %~xc gives the file extension of a given file, %c in this case, so if %c is a.cc, %~xc means .cc
# %~nc gives the file name of a given file without extension, %c in this case, so if %c is a.cc, %~nc means a

NULL=

# For HarfBuzz
!if [call create-lists.bat header hb_objs.mak harfbuzz_dll_OBJS]
!endif

!if [for %c in ($(HB_SOURCES)) do @if "%~xc" == ".cc" @call create-lists.bat file hb_objs.mak ^$(CFG)\^$(PLAT)\harfbuzz\%~nc.obj]
!endif

!if [for %c in ($(HB_SOURCES)) do @if "%~xc" == ".c" @call create-lists.bat file hb_objs.mak ^$(CFG)\^$(PLAT)\harfbuzz\%~nc.obj]
!endif

!if [call create-lists.bat footer hb_objs.mak]
!endif

# For HarfBuzz-GObject
!if "$(GOBJECT)" == "1"

!if [call create-lists.bat header hb_objs.mak harfbuzz_gobject_OBJS]
!endif

!if [for %c in ($(HB_GOBJECT_sources) $(HB_GOBJECT_ENUM_sources)) do @if "%~xc" == ".cc" @call create-lists.bat file hb_objs.mak ^$(CFG)\^$(PLAT)\harfbuzz-gobject\%~nc.obj]
!endif

!if [call create-lists.bat footer hb_objs.mak]
!endif
!endif

# For HarfBuzz-ICU
!if "$(ICU)" == "1"

!if [call create-lists.bat header hb_objs.mak harfbuzz_icu_OBJS]
!endif

!if [for %c in ($(HB_ICU_sources)) do @if "%~xc" == ".cc" @call create-lists.bat file hb_objs.mak ^$(CFG)\^$(PLAT)\harfbuzz-icu\%~nc.obj]
!endif

!if [call create-lists.bat footer hb_objs.mak]
!endif
!endif

# For the utility programs (GLib support is required)
!if "$(GLIB)" == "1"

# For hb-view, Cairo-FT support is required
!if "$(CAIRO_FT)" == "1"

!if [call create-lists.bat header hb_objs.mak hb_view_OBJS]
!endif

!if [for %c in ($(HB_VIEW_sources)) do @if "%~xc" == ".cc" @call create-lists.bat file hb_objs.mak ^$(CFG)\^$(PLAT)\util\%~nc.obj]
!endif

!if [call create-lists.bat footer hb_objs.mak]
!endif
!endif

# For hb-shape
!if [call create-lists.bat header hb_objs.mak hb_shape_OBJS]
!endif

!if [for %c in ($(HB_SHAPE_sources)) do @if "%~xc" == ".cc" @call create-lists.bat file hb_objs.mak ^$(CFG)\^$(PLAT)\util\%~nc.obj]
!endif

!if [call create-lists.bat footer hb_objs.mak]
!endif

# For hb-ot-shape-closure

!if [call create-lists.bat header hb_objs.mak hb_ot_shape_closure_OBJS]
!endif

!if [for %c in ($(HB_OT_SHAPE_CLOSURE_sources)) do @if "%~xc" == ".cc" @call create-lists.bat file hb_objs.mak ^$(CFG)\^$(PLAT)\util\%~nc.obj]
!endif

!if [call create-lists.bat footer hb_objs.mak]
!endif

!endif

!include hb_objs.mak

!if [del /f /q hb_objs.mak]
!endif

# Gather the list of headers and sources for introspection and glib-mkenums
!if [call create-lists.bat header hb_srcs.mak HB_ACTUAL_HEADERS]
!endif

!if [for %h in ($(HB_HEADERS)) do @call create-lists.bat file hb_srcs.mak ..\src\%h]
!endif

!if [call create-lists.bat footer hb_srcs.mak]
!endif

# Gather the lists of sources for introspection
!if [call create-lists.bat header hb_srcs.mak HB_ACTUAL_SOURCES]
!endif

!if [for %s in ($(HB_SOURCES)) do @call create-lists.bat file hb_srcs.mak ..\src\%s]
!endif

!if [call create-lists.bat footer hb_srcs.mak]
!endif

!if [call create-lists.bat header hb_srcs.mak HB_GOBJECT_ACTUAL_SOURCES]
!endif

!if [for %s in ($(HB_GOBJECT_sources) $(HB_GOBJECT_STRUCTS_headers)) do @call create-lists.bat file hb_srcs.mak ..\src\%s]
!endif

!if [call create-lists.bat footer hb_srcs.mak]
!endif

!include hb_srcs.mak

!if [del /f /q hb_srcs.mak]
!endif
