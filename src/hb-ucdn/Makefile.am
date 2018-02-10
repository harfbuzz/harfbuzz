## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libhb-ucdn.la

include Makefile.sources

if OS_WIN32
libhb_ucdn_la_LINK = $(CXXLINK)
else
# Use a C linker for GCC, not C++; Don't link to libstdc++
if HAVE_GCC
libhb_ucdn_la_LINK = $(LINK)
else
libhb_ucdn_la_LINK = $(CXXLINK)
endif
endif

libhb_ucdn_la_SOURCES = $(LIBHB_UCDN_sources)
libhb_ucdn_la_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src
libhb_ucdn_la_LIBADD =

EXTRA_DIST = README COPYING

-include $(top_srcdir)/git.mk
