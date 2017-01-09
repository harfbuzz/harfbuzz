# NMake Makefile portion for enabling features for Windows builds

# You may change these lines to customize the .lib files that will be linked to
# Additional Libraries for building HarfBuzz-ICU
# icudt.lib may be required for static ICU builds
HB_ICU_DEP_LIBS = icuuc.lib

# GLib is required for all utility programs and tests
HB_GLIB_LIBS = glib-2.0.lib

# Needed for building HarfBuzz-GObject
HB_GOBJECT_DEP_LIBS = gobject-2.0.lib $(HB_GLIB_LIBS)

# Freetype is needed for building FreeType support and hb-view
!if "$(CFG)" == "debug"
FREETYPE_LIB = freetyped.lib
!else
FREETYPE_LIB = freetype.lib
!endif

# Cairo is needed for building hb-view
CAIRO_LIB = cairo.lib

# Graphite2 is needed for building SIL Graphite2 support
GRAPHITE2_LIB = graphite2.lib

# Uniscribe is needed for Uniscribe shaping support
UNISCRIBE_LIB = usp10.lib gdi32.lib rpcrt4.lib user32.lib

# Directwrite is needed for DirectWrite shaping support
DIRECTWRITE_LIB = dwrite.lib

# Please do not change anything beneath this line unless maintaining the NMake Makefiles
# Bare minimum features and sources built into HarfBuzz on Windows
HB_DEFINES =
HB_CFLAGS = /DHAVE_CONFIG_H
HB_UCDN_CFLAGS = /I..\src\hb-ucdn
HB_SOURCES =	\
	$(HB_BASE_sources)		\
	$(HB_FALLBACK_sources)	\
	$(HB_OT_sources)

HB_HEADERS =	\
	$(HB_BASE_headers)		\
	$(HB_NODIST_headers)	\
	$(HB_OT_headers)

# Minimal set of (system) libraries needed for the HarfBuzz DLL
HB_DEP_LIBS =

# We build the HarfBuzz DLL/LIB at least
HB_LIBS = $(CFG)\$(PLAT)\harfbuzz.lib

# Note: All the utility and test programs require GLib support to be present!
HB_UTILS =
HB_UTILS_DEP_LIBS = $(HB_GLIB_LIBS)
HB_TESTS =
HB_TESTS_DEP_LIBS = $(HB_GLIB_LIBS)

# Use libtool-style DLL names, if desired
!if "$(LIBTOOL_DLL_NAME)" == "1"
HARFBUZZ_DLL_FILENAME = $(CFG)\$(PLAT)\libharfbuzz-0
HARFBUZZ_GOBJECT_DLL_FILENAME = $(CFG)\$(PLAT)\libharfbuzz-gobject-0
!else
HARFBUZZ_DLL_FILENAME = $(CFG)\$(PLAT)\harfbuzz-vs$(VSVER)
HARFBUZZ_GOBJECT_DLL_FILENAME = $(CFG)\$(PLAT)\harfbuzz-gobject-vs$(VSVER)
!endif

# Enable Introspection (enables HarfBuzz-Gobject as well)
!if "$(INTROSPECTION)" == "1"
GOBJECT = 1
CHECK_PACKAGE = gobject-2.0
EXTRA_TARGETS = $(CFG)\$(PLAT)\HarfBuzz-0.0.gir $(CFG)\$(PLAT)\HarfBuzz-0.0.typelib
!else
EXTRA_TARGETS =
!endif

# Enable HarfBuzz-GObject (enables GLib support as well)
!if "$(GOBJECT)" == "1"
GLIB = 1
HB_LIBS =	\
	$(HB_LIBS)	\
	$(CFG)\$(PLAT)\harfbuzz-gobject.lib

HB_GOBJECT_ENUM_GENERATED_SOURCES = \
	$(CFG)\$(PLAT)\harfbuzz-gobject\hb-gobject-enums.cc	\
	$(CFG)\$(PLAT)\harfbuzz-gobject\hb-gobject-enums.h

!endif

# Enable cairo-ft (enables cairo and freetype as well)
!if "$(CAIRO_FT)" == "1"
HB_DEFINES = $(HB_DEFINES) /DHAVE_CAIRO_FT=1
CAIRO = 1
FREETYPE = 1
!if "$(GLIB)" == "1"
HB_UTILS = \
	$(HB_UTILS)	\
	$(CFG)\$(PLAT)\hb-view.exe

HB_UTILS_DEP_LIBS = $(HB_UTILS_DEP_LIBS) $(CAIRO_LIB) $(FREETYPE_LIB)
!else
!if [echo Warning: GLib support not enabled, hb-view not built]
!endif
!endif
!endif

# Enable cairo
!if "$(CAIRO)" == "1"
HB_DEFINES = $(HB_DEFINES) /DHAVE_CAIRO=1
!endif

# Enable freetype if desired
!if "$(FREETYPE)" == "1"
!if "$(FREETYPE_DIR)" != ""
HB_CFLAGS = $(HB_CFLAGS) /I$(FREETYPE_DIR)
!endif
HB_DEFINES = $(HB_DEFINES) /DHAVE_FREETYPE=1
HB_SOURCES = $(HB_SOURCES) $(HB_FT_sources)
HB_HEADERS = $(HB_HEADERS) $(HB_FT_headers)
HB_DEP_LIBS = $(HB_DEP_LIBS) $(FREETYPE_LIB)
!endif

# Enable graphite2 if desired
!if "$(GRAPHITE2)" == "1"
HB_DEFINES = $(HB_DEFINES) /DHAVE_GRAPHITE2=1
HB_SOURCES = $(HB_SOURCES) $(HB_GRAPHITE2_sources)
HB_HEADERS = $(HB_HEADERS) $(HB_GRAPHITE2_headers)
HB_DEP_LIBS = $(HB_DEP_LIBS) $(GRAPHITE2_LIB)
!endif

# Enable GLib if desired
!if "$(GLIB)" == "1"
HB_DEFINES = $(HB_DEFINES) /DHAVE_GLIB=1
HB_CFLAGS =	\
	$(HB_CFLAGS)					\
	/FImsvc_recommended_pragmas.h	\
	/I$(PREFIX)\include\glib-2.0	\
	/I$(PREFIX)\lib\glib-2.0\include

HB_SOURCES = $(HB_SOURCES) $(HB_GLIB_sources)
HB_HEADERS = $(HB_HEADERS) $(HB_GLIB_headers)
HB_DEP_LIBS = $(HB_DEP_LIBS) $(HB_GLIB_LIBS)

HB_UTILS = \
	$(HB_UTILS)					\
	$(CFG)\$(PLAT)\hb-shape.exe	\
	$(CFG)\$(PLAT)\hb-ot-shape-closure.exe

HB_TESTS = \
	$(HB_TESTS)	\
	$(CFG)\$(PLAT)\main.exe						\
	$(CFG)\$(PLAT)\test.exe						\
	$(CFG)\$(PLAT)\test-buffer-serialize.exe	\
	$(CFG)\$(PLAT)\test-size-params.exe			\
	$(CFG)\$(PLAT)\test-would-substitute.exe	\
	$(CFG)\$(PLAT)\test-blob.exe				\
	$(CFG)\$(PLAT)\test-buffer.exe				\
	$(CFG)\$(PLAT)\test-common.exe				\
	$(CFG)\$(PLAT)\test-font.exe				\
	$(CFG)\$(PLAT)\test-object.exe				\
	$(CFG)\$(PLAT)\test-set.exe					\
	$(CFG)\$(PLAT)\test-shape.exe				\
	$(CFG)\$(PLAT)\test-unicode.exe				\
	$(CFG)\$(PLAT)\test-version.exe

!elseif "$(ICU)" == "1"
# use ICU for Unicode functions
# and define some of the macros in GLib's msvc_recommended_pragmas.h
# to reduce some unneeded build-time warnings
HB_DEFINES = $(HB_DEFINES) /DHAVE_ICU=1 /DHAVE_ICU_BUILTIN=1
HB_CFLAGS =	\
	$(HB_CFLAGS)					\
	/wd4244							\
	/D_CRT_SECURE_NO_WARNINGS		\
	/D_CRT_NONSTDC_NO_WARNINGS

# We don't want ICU to re-define int8_t in VS 2008, will cause build breakage
# as we define it in hb-common.h, and we ought to use the definitions there.
!if "$(VSVER)" == "9"
HB_CFLAGS =	$(HB_CFLAGS) /DU_HAVE_INT8_T
!endif

HB_SOURCES = $(HB_SOURCES) $(HB_ICU_sources)
HB_HEADERS = $(HB_HEADERS) $(HB_ICU_headers)
HB_DEP_LIBS = $(HB_DEP_LIBS) $(HB_ICU_DEP_LIBS)
!endif

!if "$(UCDN)" != "0"
# Define some of the macros in GLib's msvc_recommended_pragmas.h
# to reduce some unneeded build-time warnings
HB_DEFINES = $(HB_DEFINES) /DHAVE_UCDN=1
HB_CFLAGS =	\
	$(HB_CFLAGS)					\
	$(HB_UCDN_CFLAGS)				\
	/wd4244							\
	/D_CRT_SECURE_NO_WARNINGS		\
	/D_CRT_NONSTDC_NO_WARNINGS

HB_SOURCES = $(HB_SOURCES) $(LIBHB_UCDN_sources) $(HB_UCDN_sources)
!endif

!if "$(UNISCRIBE)" == "1"
HB_CFLAGS = $(HB_CFLAGS) /DHAVE_UNISCRIBE
HB_SOURCES = $(HB_SOURCES) $(HB_UNISCRIBE_sources)
HB_HEADERS = $(HB_HEADERS) $(HB_UNISCRIBE_headers)
HB_DEP_LIBS = $(HB_DEP_LIBS) $(UNISCRIBE_LIB)
!endif

!if "$(DIRECTWRITE)" == "1"
HB_CFLAGS = $(HB_CFLAGS) /DHAVE_DIRECTWRITE
HB_SOURCES = $(HB_SOURCES) $(HB_DIRECTWRITE_sources)
HB_HEADERS = $(HB_HEADERS) $(HB_DIRECTWRITE_headers)
HB_DEP_LIBS = $(HB_DEP_LIBS) $(DIRECTWRITE_LIB)
!endif

HB_LIB_CFLAGS = $(HB_CFLAGS) /DHB_EXTERN="__declspec (dllexport) extern"
