# Process this file with automake to produce Makefile.in

NULL =
EXTRA_DIST =
CLEANFILES =
SUBDIRS =

EXTRA_DIST += \
	$(TESTS) \
	expected/basics \
	expected/full-font \
	expected/cff-full-font \
	expected/japanese \
	expected/cff-japanese \
	expected/layout \
	expected/layout.gpos \
	expected/layout.gpos2 \
	expected/layout.gpos3 \
	expected/layout.gpos4 \
	expected/layout.gpos6 \
	expected/layout.gpos8 \
	expected/layout.gsub3 \
	expected/layout.gsub6 \
	expected/layout.gdef \
	expected/layout.context \
	expected/layout.gdef-varstore \
	expected/layout.gdef-attachlist \
	expected/cmap \
	expected/cmap14 \
	expected/sbix \
	expected/colr \
	expected/cbdt \
	fonts \
	profiles \
	$(NULL)

# Convenience targets:
lib:
	@$(MAKE) $(AM_MAKEFLAGS) -C $(top_builddir)/src lib

TEST_EXTENSIONS = .tests
TESTS_LOG_COMPILER = $(srcdir)/../run-tests.py $(top_builddir)/util/hb-subset$(EXEEXT)
include Makefile.sources

-include $(top_srcdir)/git.mk
