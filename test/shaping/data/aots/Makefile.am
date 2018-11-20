# Process this file with automake to produce Makefile.in

NULL =

# Convenience targets:
lib:
	@$(MAKE) $(AM_MAKEFLAGS) -C $(top_builddir)/src lib

EXTRA_DIST = \
	COPYING \
	fonts \
	$(TESTS) \
	$(NULL)

TEST_EXTENSIONS = .tests
TESTS_LOG_COMPILER = $(srcdir)/../../run-tests.py $(top_builddir)/util/hb-shape$(EXEEXT)

init-aots:
	git clone https://github.com/adobe-type-tools/aots
	make -Caots
	make -Caots/harfbuzz
	touch init-aots

check-aots: init-aots lib
	$(CXX) -Wno-narrowing aots/harfbuzz/hb-aots-tester.cpp \
		-I$(top_srcdir)/src/ -o aots/harfbuzz/aots \
		-L$(top_builddir)/src/.libs -lharfbuzz
	export LD_LIBRARY_PATH=$(realpath $(top_builddir)/src/.libs); cd aots/harfbuzz; ./aots

.PHONY: check-aots

include Makefile.sources

-include $(top_srcdir)/git.mk
