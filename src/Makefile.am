# Process this file with automake to produce Makefile.in

NULL =

# The following warning options are useful for debugging: -Wpadded -Wcast-align
#AM_CXXFLAGS =

lib_LTLIBRARIES = libharfbuzz.la

HBSOURCES =  \
	hb-blob.c \
	hb-buffer.c \
	hb-buffer-private.h \
	hb-font.cc \
	hb-font-private.h \
	hb-private.h \
	hb-shape.h \
	hb-shape.c \
	hb-unicode.c \
	hb-unicode.h \
	hb-unicode-private.h \
	hb-open-file-private.hh \
	hb-open-type-private.hh \
	hb-ot-layout.cc \
	hb-ot-layout-common-private.hh \
	hb-ot-layout-gdef-private.hh \
	hb-ot-layout-gpos-private.hh \
	hb-ot-layout-gsubgpos-private.hh \
	hb-ot-layout-gsub-private.hh \
	hb-ot-layout-private.h \
	hb-object-private.h \
	$(NULL)
HBHEADERS = \
	hb.h \
	hb-blob.h \
	hb-buffer.h \
	hb-common.h \
	hb-font.h \
	hb-ot.h \
	hb-ot-layout.h \
	$(NULL)

if HAVE_GLIB
HBSOURCES += \
	hb-glib.c \
	$(NULL)
HBHEADERS += \
	hb-glib.h \
	$(NULL)
endif

CXXLINK = $(LINK)

libharfbuzz_la_SOURCES = $(HBSOURCES) $(HBHEADERS)
libharfbuzz_la_CPPFLAGS = $(GLIB_CFLAGS)
libharfbuzz_la_LIBADD = $(GLIB_LIBS)
pkginclude_HEADERS = $(HBHEADERS)

noinst_PROGRAMS = main

main_SOURCES = main.cc
main_CPPFLAGS = $(GLIB_CFLAGS)
main_LDADD = libharfbuzz.la $(GLIB_LIBS)

TESTS = \
	check-libstdc++.sh \
	$(NULL)

-include $(top_srcdir)/git.mk
