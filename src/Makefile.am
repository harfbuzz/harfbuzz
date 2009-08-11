## Process this file with automake to produce Makefile.in

NULL =

# The following warning options are useful for debugging: -Wpadded -Wcast-align
AM_CXXFLAGS = -fno-rtti -fno-exceptions

noinst_LTLIBRARIES = libharfbuzz.la

HBSOURCES =  \
	hb-blob.c \
	hb-buffer.c \
	hb-buffer-private.h \
	hb-font.cc \
	hb-font-private.h \
	hb-private.h \
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

libharfbuzz_la_SOURCES = $(HBSOURCES) $(HBHEADERS)
libharfbuzz_la_CPPFLAGS = -DHAVE_GLIB $(GLIB_CFLAGS)
libharfbuzz_la_LIBADD = $(GLIB_LIBS)

noinst_PROGRAMS = main

main_SOURCES = main.cc
main_CPPFLAGS = -DHAVE_GLIB $(GLIB_CFLAGS)
main_LDADD = libharfbuzz.la $(GLIB_LIBS)

EXTRA_DIST = README COPYING

-include $(top_srcdir)/git.mk
