# Process this file with automake to produce Makefile.in

NULL =
EXTRA_DIST =

# The following warning options are useful for debugging: -Wpadded -Wcast-align
#AM_CXXFLAGS =

lib_LTLIBRARIES = libharfbuzz.la

HBCFLAGS =
HBLIBS =
HBSOURCES =  \
	hb-blob.cc \
	hb-blob-private.hh \
	hb-buffer.cc \
	hb-buffer-private.hh \
	hb-common.cc \
	hb-font.cc \
	hb-font-private.hh \
	hb-object-private.hh \
	hb-open-file-private.hh \
	hb-open-type-private.hh \
	hb-ot-head-private.hh \
	hb-private.hh \
	hb-shape.cc \
	hb-unicode.cc \
	hb-unicode-private.hh \
	$(NULL)
HBHEADERS = \
	hb.h \
	hb-blob.h \
	hb-buffer.h \
	hb-common.h \
	hb-font.h \
	hb-shape.h \
	hb-unicode.h \
	$(NULL)

HBSOURCES += \
	hb-ot-layout.cc \
	hb-ot-layout-common-private.hh \
	hb-ot-layout-gdef-private.hh \
	hb-ot-layout-gpos-private.hh \
	hb-ot-layout-gsubgpos-private.hh \
	hb-ot-layout-gsub-private.hh \
	hb-ot-layout-private.hh \
	hb-ot-map.cc \
	hb-ot-map-private.hh \
	hb-ot-shape.cc \
	hb-ot-shape-complex-arabic.cc \
	hb-ot-shape-complex-arabic-table.hh \
	hb-ot-shape-complex-private.hh \
	hb-ot-shape-private.hh \
	hb-ot-tag.cc \
	$(NULL)
HBHEADERS += \
	hb-ot.h \
	hb-ot-layout.h \
	hb-ot-shape.h \
	hb-ot-tag.h \
	$(NULL)

if HAVE_GLIB
HBCFLAGS += $(GLIB_CFLAGS)
HBLIBS   += $(GLIB_LIBS)
HBSOURCES += \
	hb-glib.cc \
	$(NULL)
HBHEADERS += \
	hb-glib.h \
	$(NULL)
endif

if HAVE_ICU
HBCFLAGS += $(ICU_CFLAGS)
HBLIBS   += $(ICU_LIBS)
HBSOURCES += \
	hb-icu.cc \
	$(NULL)
HBHEADERS += \
	hb-icu.h \
	$(NULL)
endif

if HAVE_FREETYPE
HBCFLAGS += $(FREETYPE_CFLAGS)
HBLIBS   += $(FREETYPE_LIBS)
HBSOURCES += \
	hb-ft.cc \
	$(NULL)
HBHEADERS += \
	hb-ft.h \
	$(NULL)
endif

CXXLINK = $(LINK)
libharfbuzz_la_SOURCES = $(HBSOURCES) $(HBHEADERS)
libharfbuzz_la_CPPFLAGS = $(HBCFLAGS)
libharfbuzz_la_LIBADD = $(HBLIBS)
pkginclude_HEADERS = $(HBHEADERS)


GENERATORS = \
	gen-arabic-joining-table.py \
	$(NULL)

EXTRA_DIST += $(GENERATORS)

noinst_PROGRAMS = main test
bin_PROGRAMS =

if HAVE_GLIB
if HAVE_FREETYPE
if HAVE_CAIRO_FT
if HAVE_CAIRO_PNG
hb_view_SOURCES = hb-view.cc
hb_view_CPPFLAGS = $(HBCFLAGS) $(CAIRO_FT_CFLAGS) $(CAIRO_PNG_CFLAGS)
hb_view_LDADD = libharfbuzz.la -lm $(HBLIBS) $(CAIRO_FT_LIBS) $(CAIRO_PNG_LIBS)
bin_PROGRAMS += hb-view
endif
endif
endif
endif

main_SOURCES = main.cc
main_CPPFLAGS = $(HBCFLAGS)
main_LDADD = libharfbuzz.la $(HBLIBS)

test_SOURCES = test.cc
test_CPPFLAGS = $(HBCFLAGS)
test_LDADD = libharfbuzz.la $(HBLIBS)

dist_check_SCRIPTS = \
	check-c-linkage-decls.sh \
	check-header-guards.sh \
	check-internal-symbols.sh \
	$(NULL)

if HAVE_ICU
else
dist_check_SCRIPTS += check-libstdc++.sh
endif

TESTS = $(dist_check_SCRIPTS)

-include $(top_srcdir)/git.mk
