
!if "$(BUILD_INTROSPECTION)" == "TRUE"
# Create the file list for introspection (to avoid the dreaded command-line-too-long problem on Windows)
$(CFG)\$(PLAT)\hb_list: $(HB_ACTUAL_HEADERS) $(HB_ACTUAL_SOURCES) $(HB_GOBJECT_ENUM_GENERATED_SOURCES) $(HB_GOBJECT_ACTUAL_SOURCES)
	@for %f in ($(HB_ACTUAL_HEADERS) $(HB_ACTUAL_SOURCES) $(HB_GOBJECT_ENUM_GENERATED_SOURCES) $(HB_GOBJECT_ACTUAL_SOURCES)) do @echo %f >> $@

$(CFG)\$(PLAT)\HarfBuzz-0.0.gir: $(CFG)\$(PLAT)\harfbuzz-gobject.lib $(CFG)\$(PLAT)\hb_list
	@set LIB=$(CFG)\$(PLAT);$(PREFIX)\lib;$(LIB)
	@set PATH=$(CFG)\$(PLAT);$(PREFIX)\bin;$(PATH)
	@-echo Generating $@...
	$(PYTHON) $(G_IR_SCANNER)	\
	--verbose -no-libtool	\
	-I..\src -n hb --identifier-prefix=hb_ --warn-all	\
	--namespace=HarfBuzz	\
	--nsversion=0.0	\
	--include=GObject-2.0	\
	--library=harfbuzz-gobject	\
	--library=harfbuzz	\
	--add-include-path=$(G_IR_INCLUDEDIR)	\
	--pkg-export=harfbuzz	\
	--cflags-begin	\
	$(CFLAGS) $(HB_DEFINES) $(HB_CFLAGS)	\
	-DHB_H \
	-DHB_H_IN \
	-DHB_OT_H \
	-DHB_OT_H_IN \
	-DHB_GOBJECT_H \
	-DHB_GOBJECT_H_IN \
	--cflags-end	\
	--filelist=$(CFG)\$(PLAT)\hb_list	\
	-o $@

$(CFG)\$(PLAT)\HarfBuzz-0.0.typelib: $(CFG)\$(PLAT)\HarfBuzz-0.0.gir
	@copy $*.gir $(@B).gir
	$(PREFIX)\bin\g-ir-compiler	\
	--includedir=$(CFG)\$(PLAT) --debug --verbose	\
	$(@B).gir	\
	-o $@
	@del $(@B).gir
!else
!error $(ERROR_MSG)
!endif
