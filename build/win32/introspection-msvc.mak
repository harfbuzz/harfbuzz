# NMake Makefile portion to generate introspection files
# One should not need to modify the contents of this
# unless maintaining the NMake build files.

G_IR_SCANNER = $(PREFIX)\bin\g-ir-scanner
G_IR_COMPILER = $(PREFIX)\bin\g-ir-compiler.exe
G_IR_INCLUDEDIR = $(PREFIX)\share\gir-1.0
G_IR_TYPELIBDIR = $(PREFIX)\lib\girepository-1.0

VALID_PKG_CONFIG_PATH = 0

MSG_INVALID_PKGCONFIG = You must set or specifiy a valid PKG_CONFIG_PATH

ERROR_MSG =

BUILD_INTROSPECTION = 1

!if ![pkg-config --print-errors --errors-to-stdout $(CHECK_PACKAGE) > pkgconfig.x]	\
	&& ![setlocal]	\
	&& ![set file="pkgconfig.x"]	\
	&& ![FOR %A IN (%file%) DO @echo PKG_CHECK_SIZE=%~zA > pkgconfig.chksize]	\
	&& ![del $(ERRNUL) /q/f pkgconfig.x]
!endif

!include pkgconfig.chksize
!if "$(PKG_CHECK_SIZE)" == "0"
VALID_PKG_CONFIG_PATH = 1
!endif

!if ![del $(ERRNUL) /q/f pkgconfig.chksize]
!endif

!if "$(VALID_PKG_CONFIG_PATH)" == "0"
BUILD_INTROSPECTION = 0
ERROR_MSG = $(MSG_INVALID_PKGCONFIG)
!endif

!if "$(BUILD_INTROSPECTION)" == "1"
# Create the file list for introspection (to avoid the dreaded command-line-too-long problem on Windows)
$(CFG)\$(PLAT)\hb_list: $(HB_HDRS) $(HB_SRCS) $(HB_GOBJECT_ENUM_GENERATED_SOURCES) $(HB_GOBJECT_SRCS)
	@for %f in ($(HB_HDRS) $(HB_SRCS) $(HB_GOBJECT_ENUM_GENERATED_SOURCES) $(HB_GOBJECT_SRCS)) do @echo %f >> $@

$(CFG)\$(PLAT)\HarfBuzz-0.0.gir: $(CFG)\$(PLAT)\harfbuzz-gobject.lib $(CFG)\$(PLAT)\hb_list
	@set LIB=$(CFG)\$(PLAT);$(PREFIX)\lib;$(LIB)
	@set PATH=$(CFG)\$(PLAT);$(PREFIX)\bin;$(PATH)
	@-echo Generating $@...
	$(PYTHON) $(G_IR_SCANNER)	\
	--verbose -no-libtool	\
	-I..\..\src -n hb --identifier-prefix=hb_ --warn-all	\
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
