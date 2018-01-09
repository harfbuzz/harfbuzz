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
	texts/in-tree \
	fonts/sha1sum \
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
	tests/misc/arabic-fallback-shaping.tests \
	tests/misc/arabic-feature-order.tests \
	tests/misc/arabic-like-joining.tests \
	tests/misc/arabic-mark-order.tests \
	tests/misc/arabic-stch.tests \
	tests/misc/automatic-fractions.tests \
	tests/misc/cluster.tests \
	tests/misc/color-fonts.tests \
	tests/misc/context-matching.tests \
	tests/misc/cursive-positioning.tests \
	tests/misc/default-ignorables.tests \
	tests/misc/emoji-flag-tags.tests \
	tests/misc/fallback-positioning.tests \
	tests/misc/fuzzed.tests \
	tests/misc/hangul-jamo.tests \
	tests/misc/hyphens.tests \
	tests/misc/indic-consonant-with-stacker.tests \
	tests/misc/indic-init.tests \
	tests/misc/indic-joiner-candrabindu.tests \
	tests/misc/indic-joiners.tests \
	tests/misc/indic-old-spec.tests \
	tests/misc/indic-pref-blocking.tests \
	tests/misc/indic-script-extensions.tests \
	tests/misc/indic-special-cases.tests \
	tests/misc/indic-syllable.tests \
	tests/misc/language-tags.tests \
	tests/misc/ligature-id.tests \
	tests/misc/mark-attachment.tests \
	tests/misc/mark-filtering-sets.tests \
	tests/misc/mongolian-variation-selector.tests \
	tests/misc/spaces.tests \
	tests/misc/simple.tests \
	tests/misc/tibetan-contractions-1.tests \
	tests/misc/tibetan-contractions-2.tests \
	tests/misc/tibetan-vowels.tests \
	tests/misc/use.tests \
	tests/misc/use-marchen.tests \
	tests/misc/use-syllable.tests \
	tests/misc/variations-rvrn.tests \
	tests/misc/vertical.tests \
	tests/misc/zero-width-marks.tests \
	$(NULL)

TEST_EXTENSIONS = \
	.tests \
	$(NULL)

AM_TESTS_ENVIRONMENT = \
	EXEEXT="$(EXEEXT)"; \
	export EXEEXT; \
	srcdir="$(srcdir)"; \
	export srcdir; \
	builddir="$(builddir)"; \
	export builddir; \
	$(NULL)

if AUTOMAKE_OLDER_THAN_1_13
TESTS_ENVIRONMENT = \
	$(AM_TESTS_ENVIRONMENT) \
	$(TESTS_LOG_COMPILER) \
	$(NULL)
endif

TESTS_LOG_COMPILER = $(srcdir)/run-tests.py

-include $(top_srcdir)/git.mk
