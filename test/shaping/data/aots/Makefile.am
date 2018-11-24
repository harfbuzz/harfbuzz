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
	git clone https://github.com/adobe-type-tools/aots $(srcdir)/aots
	make -C$(srcdir)/aots
	make -C$(srcdir)/aots/harfbuzz
	touch $(srcdir)/init-aots

update-tests: init-aots lib
	cp $(srcdir)/hb-aots-tester.cpp $(srcdir)/aots/harfbuzz/hb-aots-tester.cpp
	$(CXX) -Wno-narrowing $(srcdir)/aots/harfbuzz/hb-aots-tester.cpp \
		-I$(top_srcdir)/src/ -o $(srcdir)/aots/harfbuzz/aots \
		-L$(top_builddir)/src/.libs -lharfbuzz
	rm -rf $(srcdir)/tests/
	mkdir $(srcdir)/tests/
	export LD_LIBRARY_PATH=$(realpath $(top_builddir)/src/.libs); cd $(srcdir)/aots/harfbuzz; ./aots

.PHONY: update-tests

include Makefile.sources

-include $(top_srcdir)/git.mk
