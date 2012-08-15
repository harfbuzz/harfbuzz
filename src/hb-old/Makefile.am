## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libhb-old.la

MAINSOURCES =  \
	harfbuzz-buffer.c \
	harfbuzz-stream.c \
	harfbuzz-gdef.c \
	harfbuzz-gpos.c \
	harfbuzz-gsub.c \
	harfbuzz-impl.c \
	harfbuzz-open.c \
	harfbuzz-shaper.cpp \
	harfbuzz-greek.c \
	harfbuzz-tibetan.c \
	harfbuzz-khmer.c \
	harfbuzz-indic.cpp \
	harfbuzz-hebrew.c \
	harfbuzz-arabic.c \
	harfbuzz-hangul.c \
	harfbuzz-myanmar.c

PUBLICHEADERS = \
	harfbuzz.h \
	harfbuzz-buffer.h \
	harfbuzz-gdef.h \
	harfbuzz-gpos.h \
	harfbuzz-gsub.h \
	harfbuzz-open.h \
	harfbuzz-global.h \
	harfbuzz-external.h \
	harfbuzz-shaper.h \
	harfbuzz-stream.h

PRIVATEHEADERS = \
	harfbuzz-impl.h \
	harfbuzz-buffer-private.h \
	harfbuzz-stream-private.h \
	harfbuzz-gdef-private.h \
	harfbuzz-gpos-private.h \
	harfbuzz-gsub-private.h \
	harfbuzz-open-private.h \
	harfbuzz-shaper-private.h

libhb_old_la_SOURCES = \
	$(MAINSOURCES) \
	$(PUBLICHEADERS) \
	$(PRIVATEHEADERS)
libhb_old_la_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src

EXTRA_DIST = README COPYING

-include $(top_srcdir)/git.mk
