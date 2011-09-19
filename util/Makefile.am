# Process this file with automake to produce Makefile.in

NULL =
EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =

bin_PROGRAMS =

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/ \
	-I$(top_builddir)/src/ \
	$(GLIB_CFLAGS) \
	$(FREETYPE_CFLAGS) \
	$(CAIRO_FT_CFLAGS) \
	$(NULL)
LDADD = \
	$(top_builddir)/src/libharfbuzz.la \
	-lm \
	$(GLIB_LIBS) \
	$(FREETYPE_LIBS) \
	$(NULL)

if HAVE_GLIB
if HAVE_FREETYPE
if HAVE_CAIRO_FT
hb_view_SOURCES = \
	hb-view.cc \
	options.cc \
	options.hh \
	helper-cairo.cc \
	helper-cairo.hh \
	view-cairo.cc \
	view-cairo.hh \
	$(NULL)
hb_view_LDADD = \
	$(LDADD) \
	$(CAIRO_LIBS) \
	$(CAIRO_FT_LIBS) \
	$(NULL)
bin_PROGRAMS += hb-view
endif
endif
endif

if HAVE_GLIB
if HAVE_FREETYPE
hb_shape_SOURCES = \
	hb-shape.cc \
	options.cc \
	options.hh \
	$(NULL)
bin_PROGRAMS += hb-shape
endif
endif

-include $(top_srcdir)/git.mk
