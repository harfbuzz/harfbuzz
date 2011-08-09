# Process this file with automake to produce Makefile.in

NULL =
EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =

bin_PROGRAMS =

if HAVE_GLIB
if HAVE_FREETYPE
if HAVE_CAIRO_FT
if HAVE_CAIRO_PNG
hb_view_SOURCES = hb-view.cc
hb_view_CPPFLAGS = -I$(top_srcdir)/src/ -I$(top_builddir)/src/ $(GLIB_CFLAGS) $(FREETYPE_CFLAGS) $(CAIRO_FT_CFLAGS) $(CAIRO_PNG_CFLAGS)
hb_view_LDADD = $(top_builddir)/src/libharfbuzz.la -lm $(GLIB_LIBS) $(FREETYPE_LIBS) $(CAIRO_FT_LIBS) $(CAIRO_PNG_LIBS)
bin_PROGRAMS += hb-view
endif
endif
endif
endif

-include $(top_srcdir)/git.mk
