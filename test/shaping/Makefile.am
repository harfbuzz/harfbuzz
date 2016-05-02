# Process this file with automake to produce Makefile.in

NULL =
EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =

# Convenience targets:
lib:
	@$(MAKE) $(AM_MAKEFLAGS) -C $(top_builddir)/src lib

manifests:
	@$(srcdir)/hb-manifest-update "$(srcdir)/texts" "$(srcdir)/fonts" "$(srcdir)/tests"

EXTRA_DIST += \
	README.md \
	hb-diff \
	hb-diff-colorize \
	hb-diff-filter-failures \
	hb-diff-ngrams \
	hb-diff-stat \
	hb-manifest-read \
	hb-manifest-update \
	hb-unicode-decode \
	hb-unicode-encode \
	hb-unicode-prettyname \
	record-test.sh \
	run-tests.sh \
	texts/in-tree \
	fonts/sha1sum \
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
	tests/arabic-fallback-shaping.tests \
	tests/arabic-feature-order.tests \
	tests/cluster.tests \
	tests/context-matching.tests \
	tests/cursive-positioning.tests \
	tests/default-ignorables.tests \
	tests/fallback-positioning.tests \
	tests/fuzzed.tests \
	tests/hangul-jamo.tests \
	tests/hyphens.tests \
	tests/indic-joiner-candrabindu.tests \
	tests/indic-old-spec.tests \
	tests/indic-pref-blocking.tests \
	tests/ligature-id.tests \
	tests/mark-filtering-sets.tests \
	tests/mongolian-variation-selector.tests \
	tests/spaces.tests \
	tests/simple.tests \
	tests/use.tests \
	tests/vertical.tests \
	tests/zero-width-marks.tests \
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

TESTS_LOG_COMPILER = sh $(srcdir)/run-tests.sh

.PHONY: manifests

-include $(top_srcdir)/git.mk
