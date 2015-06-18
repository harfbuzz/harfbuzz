# Process this file with automake to produce Makefile.in

NULL =
EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =

# Convenience targets:
lib:
	@$(MAKE) $(AM_MAKEFLAGS) -C $(top_builddir)/src lib

bin_PROGRAMS =

AM_CPPFLAGS = \
	-DHB_DISABLE_DEPRECATED \
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
	main-font-text.hh \
	shape-consumer.hh \
	ansi-print.cc \
	ansi-print.hh \
	helper-cairo.cc \
	helper-cairo.hh \
	helper-cairo-ansi.cc \
	helper-cairo-ansi.hh \
	view-cairo.cc \
	view-cairo.hh \
	$(NULL)
hb_view_LDADD = \
	$(LDADD) \
	$(CAIRO_LIBS) \
	$(CAIRO_FT_LIBS) \
	$(NULL)
bin_PROGRAMS += hb-view
endif # HAVE_CAIRO_FT
endif # HAVE_FREETYPE

hb_shape_SOURCES = \
	hb-shape.cc \
	options.cc \
	options.hh \
	main-font-text.hh \
	shape-consumer.hh \
	$(NULL)
bin_PROGRAMS += hb-shape

if HAVE_OT
hb_ot_shape_closure_SOURCES = \
	hb-ot-shape-closure.cc \
	options.cc \
	options.hh \
	main-font-text.hh \
	$(NULL)
bin_PROGRAMS += hb-ot-shape-closure
endif # HAVE_OT

endif # HAVE_GLIB

#if HAVE_OT
#if HAVE_FONTCONFIG
#hb_fc_list_SOURCES = \
#	hb-fc.cc \
#	hb-fc.h \
#	hb-fc-list.c \
#	$(NULL)
#hb_fc_list_LDADD = \
#	$(LDADD) \
#	$(FONTCONFIG_LIBS) \
#	$(NULL)
#bin_PROGRAMS += hb-fc-list
#endif # HAVE_FONTCONFIG
#endif # HAVE_OT

-include $(top_srcdir)/git.mk
