# NMake Makefile portion for compilation rules
# Items in here should not need to be edited unless
# one is maintaining the NMake build files.  The format
# of NMake Makefiles here are different from the GNU
# Makefiles.  Please see the comments about these formats.

# Inference rules for compiling the .obj files.
# Used for libs and programs with more than a single source file.
# Format is as follows
# (all dirs must have a trailing '\'):
#
# {$(srcdir)}.$(srcext){$(destdir)}.obj::
# 	$(CC)|$(CXX) $(cflags) /Fo$(destdir) /c @<<
# $<
# <<
{..\src\}.cc{$(CFG)\$(PLAT)\harfbuzz\}.obj::
	$(CXX) $(CFLAGS) $(HB_DEFINES) $(HB_LIB_CFLAGS) /Fo$(CFG)\$(PLAT)\harfbuzz\ /c @<<
$<
<<

{..\src\hb-ucdn\}.c{$(CFG)\$(PLAT)\harfbuzz\}.obj::
	$(CC) $(CFLAGS) /Fo$(CFG)\$(PLAT)\harfbuzz\ /c @<<
$<
<<

{..\src\}.cc{$(CFG)\$(PLAT)\harfbuzz-icu\}.obj::
	$(CXX) $(CFLAGS) $(HB_LIB_CFLAGS) $(HB_ICU_CFLAGS) /Fo$(CFG)\$(PLAT)\harfbuzz-icu\ /c @<<
$<
<<

{..\util\}.cc{$(CFG)\$(PLAT)\util\}.obj::
	$(CXX) $(CFLAGS) $(HB_DEFINES) $(HB_CFLAGS) /Fo$(CFG)\$(PLAT)\util\ /c @<<
$<
<<

# Inference rules for building the test programs
# Used for programs with a single source file.
# Format is as follows
# (all dirs must have a trailing '\'):
#
# {$(srcdir)}.$(srcext){$(destdir)}.exe::
# 	$(CC)|$(CXX) $(cflags) $< /Fo$*.obj  /Fe$@ [/link $(linker_flags) $(dep_libs)]
{..\src\}.cc{$(CFG)\$(PLAT)\}.exe:
	$(CXX) $(CFLAGS) $(HB_DEFINES) $(HB_CFLAGS) $< /Fo$*.obj  /Fe$@ /link $(LDFLAGS) $(CFG)\$(PLAT)\harfbuzz.lib $(HB_TESTS_DEP_LIBS)

{..\test\api\}.c{$(CFG)\$(PLAT)\}.exe:
	$(CXX) $(CFLAGS) $(HB_DEFINES) $(HB_CFLAGS) /DSRCDIR="\"../../../test/api\"" $< /Fo$*.obj /Fe$@ /link $(LDFLAGS) $(CFG)\$(PLAT)\harfbuzz.lib $(HB_TESTS_DEP_LIBS)

# Rules for building .lib files
$(CFG)\$(PLAT)\harfbuzz.lib: $(HARFBUZZ_DLL_FILENAME).dll
$(CFG)\$(PLAT)\harfbuzz-icu.lib: $(HARFBUZZ_ICU_DLL_FILENAME).dll
$(CFG)\$(PLAT)\harfbuzz-gobject.lib: $(HARFBUZZ_GOBJECT_DLL_FILENAME).dll

# Rules for linking DLLs
# Format is as follows (the mt command is needed for MSVC 2005/2008 builds):
# $(dll_name_with_path): $(dependent_libs_files_objects_and_items)
#	link /DLL [$(linker_flags)] [$(dependent_libs)] [/def:$(def_file_if_used)] [/implib:$(lib_name_if_needed)] -out:$@ @<<
# $(dependent_objects)
# <<
# 	@-if exist $@.manifest mt /manifest $@.manifest /outputresource:$@;2
$(HARFBUZZ_DLL_FILENAME).dll: config.h $(harfbuzz_dll_OBJS) $(CFG)\$(PLAT)\harfbuzz
	link /DLL $(LDFLAGS) $(HB_DEP_LIBS) /implib:$(CFG)\$(PLAT)\harfbuzz.lib -out:$@ @<<
$(harfbuzz_dll_OBJS)
<<
	@-if exist $@.manifest mt /manifest $@.manifest /outputresource:$@;2

$(HARFBUZZ_ICU_DLL_FILENAME).dll: $(CFG)\$(PLAT)\harfbuzz.lib $(harfbuzz_icu_OBJS) $(CFG)\$(PLAT)\harfbuzz-icu
	link /DLL $(LDFLAGS) $(CFG)\$(PLAT)\harfbuzz.lib $(HB_ICU_DEP_LIBS) /implib:$(CFG)\$(PLAT)\harfbuzz-icu.lib -out:$@ @<<
$(harfbuzz_icu_OBJS)
<<
	@-if exist $@.manifest mt /manifest $@.manifest /outputresource:$@;2

$(HARFBUZZ_GOBJECT_DLL_FILENAME).dll: $(CFG)\$(PLAT)\harfbuzz.lib $(harfbuzz_gobject_OBJS) $(CFG)\$(PLAT)\harfbuzz-gobject
	link /DLL $(LDFLAGS) $(CFG)\$(PLAT)\harfbuzz.lib $(HB_GOBJECT_DEP_LIBS) /implib:$(CFG)\$(PLAT)\harfbuzz-gobject.lib -out:$@ @<<
$(harfbuzz_gobject_OBJS)
<<
	@-if exist $@.manifest mt /manifest $@.manifest /outputresource:$@;2

# Rules for linking Executables
# Format is as follows (the mt command is needed for MSVC 2005/2008 builds):
# $(dll_name_with_path): $(dependent_libs_files_objects_and_items)
#	link [$(linker_flags)] [$(dependent_libs)] -out:$@ @<<
# $(dependent_objects)
# <<
# 	@-if exist $@.manifest mt /manifest $@.manifest /outputresource:$@;1
$(CFG)\$(PLAT)\hb-view.exe: $(CFG)\$(PLAT)\harfbuzz.lib $(CFG)\$(PLAT)\util $(hb_view_OBJS)
	link $(LDFLAGS) $(CFG)\$(PLAT)\harfbuzz.lib $(HB_UTILS_DEP_LIBS) -out:$@ @<<
$(hb_view_OBJS)
<<
	@-if exist $@.manifest mt /manifest $@.manifest /outputresource:$@;1

$(CFG)\$(PLAT)\hb-shape.exe: $(CFG)\$(PLAT)\harfbuzz.lib $(CFG)\$(PLAT)\util $(hb_shape_OBJS)
	link $(LDFLAGS) $(CFG)\$(PLAT)\harfbuzz.lib $(HB_UTILS_DEP_LIBS) -out:$@ @<<
$(hb_shape_OBJS)
<<
	@-if exist $@.manifest mt /manifest $@.manifest /outputresource:$@;1

$(CFG)\$(PLAT)\hb-ot-shape-closure.exe: $(CFG)\$(PLAT)\harfbuzz.lib $(CFG)\$(PLAT)\util $(hb_ot_shape_closure_OBJS)
	link $(LDFLAGS) $(CFG)\$(PLAT)\harfbuzz.lib $(HB_UTILS_DEP_LIBS) -out:$@ @<<
$(hb_ot_shape_closure_OBJS)
<<
	@-if exist $@.manifest mt /manifest $@.manifest /outputresource:$@;1

# Other .obj files requiring individual attention, that could not be covered by the inference rules.
# Format is as follows (all dirs must have a trailing '\'):
#
# $(obj_file):
# 	$(CC)|$(CXX) $(cflags) /Fo$(obj_destdir) /c @<<
# $(srcfile)
# <<
$(CFG)\$(PLAT)\harfbuzz-gobject\hb-gobject-structs.obj:	$(CFG)\$(PLAT)\harfbuzz-gobject $(HB_GOBJECT_ENUM_GENERATED_SOURCES)
	$(CXX) $(CFLAGS) $(HB_DEFINES) $(HB_LIB_CFLAGS) /I$(CFG)\$(PLAT)\harfbuzz-gobject /Fo$(CFG)\$(PLAT)\harfbuzz-gobject\ /c @<<
..\src\hb-gobject-structs.cc
<<

$(CFG)\$(PLAT)\harfbuzz-gobject\hb-gobject-enums.obj: $(CFG)\$(PLAT)\harfbuzz-gobject $(HB_GOBJECT_ENUM_GENERATED_SOURCES)
	$(CXX) $(CFLAGS) $(HB_DEFINES) $(HB_LIB_CFLAGS) /I$(CFG)\$(PLAT)\harfbuzz-gobject /Fo$(CFG)\$(PLAT)\harfbuzz-gobject\ /c @<<
$(CFG)\$(PLAT)\harfbuzz-gobject\hb-gobject-enums.cc
<<

clean:
	@-if exist $(CFG)\$(PLAT)\HarfBuzz-0.0.typelib del /f /q $(CFG)\$(PLAT)\HarfBuzz-0.0.typelib
	@-if exist $(CFG)\$(PLAT)\HarfBuzz-0.0.gir del /f /q $(CFG)\$(PLAT)\HarfBuzz-0.0.gir
	@-if exist $(CFG)\$(PLAT)\hb_list del /f /q $(CFG)\$(PLAT)\hb_list
	@-del /f /q $(CFG)\$(PLAT)\*.pdb
	@-if exist $(CFG)\$(PLAT)\.exe.manifest del /f /q $(CFG)\$(PLAT)\*.exe.manifest
	@-if exist $(CFG)\$(PLAT)\.exe del /f /q $(CFG)\$(PLAT)\*.exe
	@-del /f /q $(CFG)\$(PLAT)\*.dll.manifest
	@-del /f /q $(CFG)\$(PLAT)\*.dll
	@-del /f /q $(CFG)\$(PLAT)\*.ilk
	@-del /f /q $(CFG)\$(PLAT)\*.obj
	@-if exist $(CFG)\$(PLAT)\util del /f /q $(CFG)\$(PLAT)\util\*.obj
	@-if exist $(CFG)\$(PLAT)\harfbuzz-gobject del /f /q $(CFG)\$(PLAT)\harfbuzz-gobject\*.obj
	@-if exist $(CFG)\$(PLAT)\harfbuzz-icu del /f /q $(CFG)\$(PLAT)\harfbuzz-icu\*.obj
	@-del /f /q $(CFG)\$(PLAT)\harfbuzz\*.obj
	@-rmdir /s /q $(CFG)\$(PLAT)
	@-if exist $(CFG)\$(PLAT)\harfbuzz-gobject\hb-gobject-enums.h del $(CFG)\$(PLAT)\harfbuzz-gobject\hb-gobject-enums.h
	@-if exist $(CFG)\$(PLAT)\harfbuzz-gobject\hb-gobject-enums.cc del $(CFG)\$(PLAT)\harfbuzz-gobject\hb-gobject-enums.cc
	@-del vc$(VSVER)0.pdb
	@-del config.h
