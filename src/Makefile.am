## Process this file with automake to produce Makefile.in

NULL =

CXX = gcc -g -O2 -fno-rtti -fno-exceptions -Wabi -Wpadded -Wcast-align

noinst_LTLIBRARIES = libharfbuzz-1.la

HBSOURCES =  \
	hb-blob.c \
	hb-buffer.c \
	hb-buffer-private.h \
	hb-font.c \
	hb-font-private.h \
	hb-private.h \
	hb-open-file-private.hh \
	hb-open-types-private.hh \
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

libharfbuzz_1_la_SOURCES = $(HBSOURCES) $(HBHEADERS)
libharfbuzz_1_la_LIBADD =

noinst_PROGRAMS = main

main_SOURCES = main.cc
main_CXXFLAGS = $(GLIB_CFLAGS)
main_LDADD = $(GLIB_LIBS)

EXTRA_DIST = README COPYING

-include $(top_srcdir)/git.mk
