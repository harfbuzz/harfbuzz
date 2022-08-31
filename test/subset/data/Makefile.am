# Process this file with automake to produce Makefile.in

NULL =
EXTRA_DIST =
CLEANFILES =
SUBDIRS = repack_tests

EXTRA_DIST += \
	$(TESTS) \
	expected/32bit_var_store \
	expected/basics \
	expected/full-font \
	expected/glyf_bug_3131 \
	expected/cff-full-font \
	expected/japanese \
	expected/cff-japanese \
	expected/cff.notoserifmyanmar \
	expected/layout \
	expected/layout.gpos \
	expected/layout.gpos2 \
	expected/layout.gpos3 \
	expected/layout.gpos4 \
	expected/layout.gpos5 \
	expected/layout.gpos6 \
	expected/layout.gpos8 \
	expected/layout.gpos8.amiri \
	expected/layout.gpos9 \
	expected/layout.gsub3 \
	expected/layout.gsub5 \
	expected/layout.gsub5_format2 \
	expected/layout.gsub6 \
	expected/layout.gsub8 \
	expected/layout.khmer \
	expected/layout.gdef \
	expected/layout.gdef.glyphset \
	expected/layout.context \
	expected/layout.context_format2 \
	expected/layout.gdef-varstore \
	expected/layout.gdef-attachlist \
	expected/layout.notonastaliqurdu \
	expected/layout.tinos \
	expected/layout.default_features \
	expected/layout.duplicate_features \
	expected/layout.unsorted_featurelist \
	expected/layout.drop_feature \
	expected/cmap \
	expected/cmap14 \
	expected/sbix \
	expected/colr \
	expected/colr_glyphs \
	expected/colrv1 \
	expected/colr_with_components \
	expected/cbdt \
	expected/variable \
	expected/glyph_names \
	expected/math \
	expected/math_coverage_offset \
	expected/post \
	fonts \
	profiles \
	$(NULL)

# TODO: re-able once colrv1 subsetting is stabilized.
#      	expected/colrv1
#   	expected/colrv1.notoemoji

# Convenience targets:
lib:
	@$(MAKE) $(AM_MAKEFLAGS) -C $(top_builddir)/src lib

TEST_EXTENSIONS = .tests
TESTS_LOG_COMPILER = $(srcdir)/../run-tests.py $(top_builddir)/util/hb-subset$(EXEEXT)
include Makefile.sources

-include $(top_srcdir)/git.mk
