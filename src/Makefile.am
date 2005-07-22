## Process this file with automake to produce Makefile.in

INCLUDES = 					\
	-DG_LOG_DOMAIN=\"Pango\"		\
	-DPANGO_ENABLE_ENGINE			\
	-DSYSCONFDIR=\"$(sysconfdir)\"		\
	-DLIBDIR=\"$(libdir)\"			\
	-DG_DISABLE_DEPRECATED			\
	$(PANGO_DEBUG_FLAGS)			\
	$(GLIB_CFLAGS)				\
	$(FREETYPE_CFLAGS)			\
	$(X_CFLAGS)				\
	-I$(top_srcdir)

noinst_LTLIBRARIES = libpango-ot.la

libpango_ot_la_SOURCES =  	\
	ftglue.h		\
	ftglue.c		\
	ftxopen.c	 	\
	ftxopen.h	 	\
	ftxopenf.h	 	\
	ftxgdef.c		\
	ftxgdef.h		\
	ftxgpos.c 		\
	ftxgpos.h		\
	ftxgsub.c		\
	ftxgsub.h		\
	otlbuffer.c		\
	otlbuffer.h		\
	pango-ot-buffer.c	\
	pango-ot-info.c		\
	pango-ot-private.h	\
	pango-ot-ruleset.c

libpango_ot_la_LIBADD = \
	$(x_ldflags)	\
	$(x_libs)	\
	$(GLIB_LIBS)	\
	-lm

if BUILD_OT_TESTS
noinst_PROGRAMS = ottest
endif

ottest_SOURCES = 	\
	ottest.c	\
	disasm.c	\
	disasm.h

ottest_LDADD = 		\
	libpango-ot.la	\
	$(FREETYPE_LIBS)

EXTRA_DIST = 		\
	README
