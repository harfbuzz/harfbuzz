# Process this file with automake to produce Makefile.in

NULL =
EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =

# Convenience targets:
lib:
	@$(MAKE) $(AM_MAKEFLAGS) -C $(top_builddir)/src fuzzing

$(top_builddir)/src/libharfbuzz-fuzzing.la: lib

EXTRA_DIST += \
	README \
	$(NULL)

check_PROGRAMS = \
	hb-fuzzer \
	$(NULL)

AM_CPPFLAGS = \
	-DHB_DISABLE_DEPRECATED \
	-I$(top_srcdir)/src/ \
	-I$(top_builddir)/src/ \
	$(NULL)
LDADD = \
	$(top_builddir)/src/libharfbuzz-fuzzing.la \
	$(NULL)

hb_fuzzer_SOURCES = \
	hb-fuzzer.hh \
	hb-fuzzer.cc \
	main.cc \
	$(NULL)
hb_fuzzer_LDADD = \
	$(LDADD) \
	$(NULL)
hb_fuzzer_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(NULL)
hb_fuzzer_DEPENDENCIES = \
	lib \
	$(NULL)

check:
	cat $(srcdir)/../shaping/tests/fuzzed.tests | \
	cut -d: -f1 | while read x; do \
		./hb-fuzzer $(srcdir)/../shaping/$$x; \
	done

-include $(top_srcdir)/git.mk
