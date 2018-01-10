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
	README.md \
	CMakeLists.txt \
	hb-diff \
	hb-diff-colorize \
	hb-diff-filter-failures \
	hb-diff-stat \
	hb-unicode-decode \
	hb-unicode-encode \
	hb-unicode-prettyname \
	record-test.sh \
	run-tests.py \
	texts/in-house \
	data/in-house/fonts \
	data/text-rendering-tests/extract-tests.py \
	data/text-rendering-tests/fonts \
	$(TESTS) \
	$(NULL)

# TODO Figure out Python stuff
EXTRA_DIST += \
	hb_test_tools.py \
	$(NULL)
CLEANFILES += \
	hb_test_tools.py[co] \
	$(NULL)

TESTS = \
	data/in-house/tests/arabic-fallback-shaping.tests \
	data/in-house/tests/arabic-feature-order.tests \
	data/in-house/tests/arabic-like-joining.tests \
	data/in-house/tests/arabic-mark-order.tests \
	data/in-house/tests/arabic-stch.tests \
	data/in-house/tests/automatic-fractions.tests \
	data/in-house/tests/cluster.tests \
	data/in-house/tests/color-fonts.tests \
	data/in-house/tests/context-matching.tests \
	data/in-house/tests/cursive-positioning.tests \
	data/in-house/tests/default-ignorables.tests \
	data/in-house/tests/emoji-flag-tags.tests \
	data/in-house/tests/fallback-positioning.tests \
	data/in-house/tests/fuzzed.tests \
	data/in-house/tests/hangul-jamo.tests \
	data/in-house/tests/hyphens.tests \
	data/in-house/tests/indic-consonant-with-stacker.tests \
	data/in-house/tests/indic-init.tests \
	data/in-house/tests/indic-joiner-candrabindu.tests \
	data/in-house/tests/indic-joiners.tests \
	data/in-house/tests/indic-old-spec.tests \
	data/in-house/tests/indic-pref-blocking.tests \
	data/in-house/tests/indic-script-extensions.tests \
	data/in-house/tests/indic-special-cases.tests \
	data/in-house/tests/indic-syllable.tests \
	data/in-house/tests/language-tags.tests \
	data/in-house/tests/ligature-id.tests \
	data/in-house/tests/mark-attachment.tests \
	data/in-house/tests/mark-filtering-sets.tests \
	data/in-house/tests/mongolian-variation-selector.tests \
	data/in-house/tests/spaces.tests \
	data/in-house/tests/simple.tests \
	data/in-house/tests/tibetan-contractions-1.tests \
	data/in-house/tests/tibetan-contractions-2.tests \
	data/in-house/tests/tibetan-vowels.tests \
	data/in-house/tests/use.tests \
	data/in-house/tests/use-marchen.tests \
	data/in-house/tests/use-syllable.tests \
	data/in-house/tests/variations-rvrn.tests \
	data/in-house/tests/vertical.tests \
	data/in-house/tests/zero-width-marks.tests \
	data/text-rendering-tests/tests/AVAR-1.tests \
	data/text-rendering-tests/tests/CFF-1.tests \
	data/text-rendering-tests/tests/CFF2-1.tests \
	data/text-rendering-tests/tests/CFF-2.tests \
	data/text-rendering-tests/tests/CMAP-1.tests \
	data/text-rendering-tests/tests/CMAP-2.tests \
	data/text-rendering-tests/tests/CVAR-1.tests \
	data/text-rendering-tests/tests/CVAR-2.tests \
	data/text-rendering-tests/tests/GLYF-1.tests \
	data/text-rendering-tests/tests/GPOS-1.tests \
	data/text-rendering-tests/tests/GPOS-2.tests \
	data/text-rendering-tests/tests/GPOS-3.tests \
	data/text-rendering-tests/tests/GPOS-4.tests \
	data/text-rendering-tests/tests/GPOS-5.tests \
	data/text-rendering-tests/tests/GSUB-1.tests \
	data/text-rendering-tests/tests/GSUB-2.tests \
	data/text-rendering-tests/tests/GVAR-1.tests \
	data/text-rendering-tests/tests/GVAR-2.tests \
	data/text-rendering-tests/tests/GVAR-3.tests \
	data/text-rendering-tests/tests/GVAR-4.tests \
	data/text-rendering-tests/tests/GVAR-5.tests \
	data/text-rendering-tests/tests/GVAR-6.tests \
	data/text-rendering-tests/tests/GVAR-7.tests \
	data/text-rendering-tests/tests/GVAR-8.tests \
	data/text-rendering-tests/tests/GVAR-9.tests \
	data/text-rendering-tests/tests/HVAR-1.tests \
	data/text-rendering-tests/tests/HVAR-2.tests \
	data/text-rendering-tests/tests/KERN-1.tests \
	data/text-rendering-tests/tests/KERN-2.tests \
	data/text-rendering-tests/tests/SHKNDA-1.tests \
	$(NULL)

DISABLED_TESTS = \
	data/text-rendering-tests/tests/CMAP-3.tests # Non-Unicode cmap \
	data/text-rendering-tests/tests/MORX-1.tests # Not implemented \
	data/text-rendering-tests/tests/MORX-2.tests # Not implemented \
	data/text-rendering-tests/tests/MORX-3.tests # Not implemented \
	data/text-rendering-tests/tests/MORX-4.tests # Not implemented \
	data/text-rendering-tests/tests/SHARAN-1.tests # Rounding difference \
	data/text-rendering-tests/tests/SHBALI-1.tests # Rounding difference \
	data/text-rendering-tests/tests/SHBALI-2.tests # Rounding difference \
	data/text-rendering-tests/tests/SHBALI-3.tests # Rounding difference \
	data/text-rendering-tests/tests/SHKNDA-2.tests # Rounding difference \
	data/text-rendering-tests/tests/SHKNDA-3.tests # Rounding difference \
	$(NULL)

TEST_EXTENSIONS = .tests
TESTS_LOG_COMPILER = $(srcdir)/run-tests.py $(top_builddir)/util/hb-shape$(EXEEXT)

-include $(top_srcdir)/git.mk
