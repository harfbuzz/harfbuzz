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
	CMakeLists.txt \
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
	$(NULL)

TEST_EXTENSIONS = \
	.tests \
	$(NULL)

AM_TESTS_ENVIRONMENT = \
	EXEEXT="$(EXEEXT)"; \
	export EXEEXT; \
	$(NULL)

if AUTOMAKE_OLDER_THAN_1_13
TESTS_ENVIRONMENT = \
	$(AM_TESTS_ENVIRONMENT) \
	$(TESTS_LOG_COMPILER) \
	$(NULL)
endif

TESTS_LOG_COMPILER = $(srcdir)/run-tests.py

-include $(top_srcdir)/git.mk
