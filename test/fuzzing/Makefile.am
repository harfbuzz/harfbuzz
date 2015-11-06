# Process this file with automake to produce Makefile.in

NULL =
EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =

# Convenience targets:
lib:
	@$(MAKE) $(AM_MAKEFLAGS) -C $(top_builddir)/src lib

EXTRA_DIST += \
	README \
	$(NULL)

noinst_PROGRAMS = \
	hb-fuzzer \
	$(NULL)
check_PROGRAMS = $(noinst_PROGRAMS)

AM_CPPFLAGS = \
	-DHB_DISABLE_DEPRECATED \
	-I$(top_srcdir)/src/ \
	-I$(top_builddir)/src/ \
	$(NULL)
LDADD = \
	$(top_builddir)/src/libharfbuzz.la \
	$(NULL)

hb_fuzzer_SOURCES = \
	hb-fuzzer.cc \
	$(NULL)
hb_fuzzer_LDADD = \
	$(LDADD) \
	$(NULL)
hb_fuzzer_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DMAIN \
	-DHB_MAX_NESTING_LEVEL=3 \
	-DHB_SANITIZE_MAX_EDITS=3 \
	-DHB_BUFFER_MAX_EXPANSION_FACTOR=3 \
	-DHB_BUFFER_MAX_LEN_MIN=8 \
	-DHB_BUFFER_MAX_LEN_DEFAULT=128 \
	$(NULL)

-include $(top_srcdir)/git.mk
