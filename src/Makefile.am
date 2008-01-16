## Process this file with automake to produce Makefile.in

INCLUDES = 					\
	$(FREETYPE_CFLAGS)			\
	-I $(srcdir)

noinst_LTLIBRARIES = libharfbuzz-1.la

MAINSOURCES =  \
	harfbuzz.c

# included from harfbuzz.c
INCLUDEDSOURCES = \
	harfbuzz-buffer.c \
	harfbuzz-gdef.c \
	harfbuzz-gpos.c \
	harfbuzz-gsub.c \
	harfbuzz-impl.c \
	harfbuzz-open.c \
	harfbuzz-stream.c

PUBLICHEADERS = \
	harfbuzz.h \
	harfbuzz-global.h \
	harfbuzz-buffer.h \
	harfbuzz-gdef.h \
	harfbuzz-gpos.h \
	harfbuzz-gsub.h \
	harfbuzz-open.h

PRIVATEHEADERS = \
	harfbuzz-impl.h \
	harfbuzz-buffer-private.h \
	harfbuzz-gdef-private.h \
	harfbuzz-gpos-private.h \
	harfbuzz-gsub-private.h \
	harfbuzz-open-private.h \
	harfbuzz-stream-private.h

libharfbuzz_1_la_SOURCES = \
	$(MAINSOURCES) \
	$(PUBLICHEADERS) \
	$(PRIVATEHEADERS)

libharfbuzz_1_la_LIBADD = \
	$(FREETYPE_LIBS)

noinst_PROGRAMS = harfbuzz-dump

harfbuzz_dump_SOURCES =	\
	harfbuzz-dump.c \
	harfbuzz-dump.h \
	harfbuzz-dump-main.c

harfbuzz_dump_LDADD = \
	$(libharfbuzz_1_la_LIBADD) \
	libharfbuzz-1.la

EXTRA_DIST = 		\
	README		\
	COPYING		\
	$(INCLUDEDSOURCES)
