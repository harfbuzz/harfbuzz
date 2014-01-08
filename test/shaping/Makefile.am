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

TESTS = tests/context-matching.tests

TEST_EXTENSIONS = \
	.tests \
	$(NULL)

AM_TESTS_ENVIRONMENT = \
	EXEEXT="$(EXEEXT)"; \
	export EXEECT; \
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

bin_SCRIPTS = hb-unicode-decode hb-unicode-encode hb-unicode-prettyname

dist_pkgpython_PYTHON = \
	__init__.py \
	hb_test_tools.py \
	hb_test_tools.pyc
