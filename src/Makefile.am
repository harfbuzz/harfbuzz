## Process this file with automake to produce Makefile.in

INCLUDES = 					\
	-DSYSCONFDIR=\"$(sysconfdir)\" 		\
	-DLIBDIR=\"$(libdir)\" 			\
	$(FREETYPE_CFLAGS)			\
	$(X_CFLAGS)				\
	-I$(top_srcdir)

LDADDS = @STRIP_BEGIN@ 	\
	@x_ldflags@	\
	@x_libs@	\
	@GLIB_LIBS@	\
	-lm		\
@STRIP_END@

noinst_LTLIBRARIES = libpango-ot.la

libpango_ot_la_SOURCES =  	\
	ftxopen.c	 	\
	ftxgdef.c		\
	ftxgpos.c 		\
	pango-ot-info.c		\
	pango-ot-ruleset.c	\
	ftxgsub.c

noinst_PROGRAMS = ottest

ottest_SOURCES = 	\
	ottest.c	\
	disasm.c	\
	disasm.h

ottest_LDADD = 		\
	libpango-ot.la	\
	$(FREETYPE_LIBS)

EXTRA_DIST = 		\
	README		\
	pango-ot.def	\
	makefile.mingw.in
