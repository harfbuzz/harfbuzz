# Process this file with automake to produce Makefile.in

NULL =
EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =

manifests:
	@$(srcdir)/hb-manifest-update "$(srcdir)/texts" "$(srcdir)/fonts" "$(srcdir)/tests"

EXTRA_DIST += \
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

TESTS = tests/context-matching.tests
TESTS_ENVIRONMENT = \
	EXEEXT="$(EXEEXT)" \
	srcdir="$(srcdir)" \
	builddir="$(builddir)" \
	$(srcdir)/run-tests.sh \
	$(NULL)

.PHONY: manifests

-include $(top_srcdir)/git.mk
