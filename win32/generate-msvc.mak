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
	-$(PYTHON) $(PREFIX)\bin\glib-mkenums \
		--identifier-prefix hb_ --symbol-prefix hb_gobject \
		--template ..\src\$(@F).tmpl  $(HB_ACTUAL_HEADERS) > $@.tmp
	for %%f in ($@.tmp) do if %%~zf gtr 0 $(PYTHON) sed-enums-srcs.py --input=$@.tmp --output=$@
	@-del $@.tmp
	if not exist $@ \
	$(PERL) $(PREFIX)\bin\glib-mkenums \
		--identifier-prefix hb_ --symbol-prefix hb_gobject \
		--template ..\src\$(@F).tmpl  $(HB_ACTUAL_HEADERS) > $@.tmp
	if exist $@.tmp $(PERL) -p -i.tmp1 -e "s/_t_get_type/_get_type/g" $@.tmp
	if exist $@.tmp $(PERL) -p -i.tmp2 -e "s/_T \(/ (/g" $@.tmp
	@if exist $@.tmp.tmp1 del $@.tmp.tmp1
	@if exist $@.tmp.tmp2 del $@.tmp.tmp2
	@if exist $@.tmp move $@.tmp $@
!endif

# Create the build directories
$(CFG)\$(PLAT)\harfbuzz $(CFG)\$(PLAT)\harfbuzz-gobject $(CFG)\$(PLAT)\util:
	@-md $@

.SUFFIXES: .c .cc .hh .rl

# Generate headers from Ragel sources
{..\src\}.rl{..\src\}.hh:
	$(RAGEL) -e -F1 -o $@ $<
