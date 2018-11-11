# Process this file with automake to produce Makefile.in

NULL =
EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =

# Convenience targets:
lib-only:
	@$(MAKE) $(AM_MAKEFLAGS) -C $(top_builddir)/src fuzzing
lib: lib-only
	@$(MAKE) $(AM_MAKEFLAGS) $(check_PROGRAMS)

$(top_builddir)/src/libharfbuzz-fuzzing.la: lib-only
$(top_builddir)/src/libharfbuzz-subset-fuzzing.la: lib-only

EXTRA_DIST += \
	README \
	run-shape-fuzzer-tests.py \
	run-subset-fuzzer-tests.py \
	CMakeLists.txt \
	fonts \
	$(NULL)

check_PROGRAMS = \
	hb-shape-fuzzer \
	hb-subset-fuzzer \
	$(NULL)

AM_CPPFLAGS = \
	-DHB_DISABLE_DEPRECATED \
	-I$(top_srcdir)/src/ \
	-I$(top_builddir)/src/ \
	$(NULL)
LDADD = \
	$(top_builddir)/src/libharfbuzz-fuzzing.la \
	$(NULL)

hb_shape_fuzzer_SOURCES = \
	hb-fuzzer.hh \
	hb-shape-fuzzer.cc \
	main.cc \
	$(NULL)
hb_shape_fuzzer_LDADD = \
	$(LDADD) \
	$(NULL)
hb_shape_fuzzer_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(NULL)
hb_shape_fuzzer_DEPENDENCIES = \
	$(top_builddir)/src/libharfbuzz-fuzzing.la
	$(NULL)

hb_subset_fuzzer_SOURCES = \
	hb-fuzzer.hh \
	hb-subset-fuzzer.cc \
	main.cc \
	$(NULL)
hb_subset_fuzzer_LDADD = \
	$(top_builddir)/src/libharfbuzz-subset-fuzzing.la \
	$(NULL)
hb_subset_fuzzer_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(NULL)
hb_subset_fuzzer_DEPENDENCIES = \
	$(top_builddir)/src/libharfbuzz-subset-fuzzing.la
	$(NULL)

check:
	EXEEXT="$(EXEEXT)" srcdir="$(srcdir)" builddir="$(builddir)" $(srcdir)/run-shape-fuzzer-tests.py
	EXEEXT="$(EXEEXT)" srcdir="$(srcdir)" builddir="$(builddir)" $(srcdir)/run-subset-fuzzer-tests.py
check-valgrind:
	$(AM_V_at)RUN_VALGRIND=1 $(MAKE) $(AM_MAKEFLGS) check

-include $(top_srcdir)/git.mk
