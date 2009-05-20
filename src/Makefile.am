## Process this file with automake to produce Makefile.in

NULL =

INCLUDES = 					\
	-I $(srcdir)				\
	$(FREETYPE_CFLAGS)			\
	$(GLIB_CFLAGS)
CXX = gcc $(GCCOPTS) -g -fno-rtti -fno-exceptions -Wabi -Wpadded -Wcast-align

noinst_LTLIBRARIES = libharfbuzz-1.la

HBSOURCES =  \
	harfbuzz-buffer.c \
	hb-private.h \
	hb-ot-layout.cc \
	hb-ot-layout-common-private.h \
	hb-ot-layout-gdef-private.h \
	hb-ot-layout-gpos-private.h \
	hb-ot-layout-gsubgpos-private.h \
	hb-ot-layout-gsub-private.h \
	hb-ot-layout-open-private.h \
	hb-ot-layout-private.h \
	$(NULL)

HBHEADERS = \
	hb-common.h \
	harfbuzz-buffer.h \
	hb-ot-layout.h \
	$(NULL)

libharfbuzz_1_la_SOURCES = $(HBSOURCES) $(HBHEADERS)
libharfbuzz_1_la_LIBADD = $(FREETYPE_LIBS)

noinst_PROGRAMS = main

main_SOURCES = main.cc
main_LDADD = $(GLIB_LIBS)

EXTRA_DIST = README COPYING

-include $(top_srcdir)/git.mk
