# NMake Makefile portion for code generation and
# intermediate build directory creation
# Items in here should not need to be edited unless
# one is maintaining the NMake build files.

# Copy the pre-defined config.h.win32
config.h: config.h.win32
	@-copy $@.win32 $@

# Generate the enumeration sources and headers
# sed is not normally available on Windows, but since
# we are already using PERL, use PERL one-liners.
!if "$(GOBJECT)" == "1"
$(HB_GOBJECT_ENUM_GENERATED_SOURCES): ..\src\hb-gobject-enums.h.tmpl ..\src\hb-gobject-enums.cc.tmpl $(HB_ACTUAL_HEADERS)
	$(PERL) $(PREFIX)\bin\glib-mkenums \
		--identifier-prefix hb_ --symbol-prefix hb_gobject \
		--template ..\src\$(@F).tmpl  $(HB_ACTUAL_HEADERS) > $@
	$(PERL) -p -i.tmp1 -e "s/_t_get_type/_get_type/g" $@
	$(PERL) -p -i.tmp2 -e "s/_T \(/ (/g" $@
	@-del $@.tmp1
	@-del $@.tmp2
!endif

# Create the build directories
$(CFG)\$(PLAT)\harfbuzz $(CFG)\$(PLAT)\harfbuzz-icu $(CFG)\$(PLAT)\harfbuzz-gobject $(CFG)\$(PLAT)\util:
	@-mkdir $@
