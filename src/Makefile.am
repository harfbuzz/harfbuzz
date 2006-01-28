## Process this file with automake to produce Makefile.in

INCLUDES = 					\
	-DSYSCONFDIR=\"$(sysconfdir)\" 		\
	-DLIBDIR=\"$(libdir)\" 			\
	$(PANGO_DEBUG_FLAGS)			\
	-I$(top_srcdir)				\
	$(GLIB_CFLAGS)				\
	$(FREETYPE_CFLAGS)

noinst_LTLIBRARIES = libmini-harfbuzz.la

libmini_harfbuzz_la_SOURCES =  	\
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
	otlbuffer.h

libmini_harfbuzz_la_LIBADD = \
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
	libmini-harfbuzz.la	\
	$(FREETYPE_LIBS)

EXTRA_DIST = 		\
	README		\
	FTL.TXT		\
	FT-license.txt

