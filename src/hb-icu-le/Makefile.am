## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libhb-icu-le.la


libhb_icu_le_la_SOURCES = \
	FontTableCache.cpp \
	FontTableCache.h \
	PortableFontInstance.cpp \
	PortableFontInstance.h \
	cmaps.cpp \
	cmaps.h \
	letest.h \
	sfnt.h
libhb_icu_le_la_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src \
	$(ICU_LE_CFLAGS)
libhb_icu_le_la_LIBADD = \
	$(ICU_LE_LIBS)

EXTRA_DIST = README license.html COPYING

-include $(top_srcdir)/git.mk
