# Process this file with automake to produce Makefile.in

NULL =
EXTRA_DIST =
CLEANFILES =
SUBDIRS =

EXTRA_DIST +=
	$(TESTS) \
	$(NULL)

# Convenience targets:
lib: libs # Always build subsetter lib in this subdir
libs:
	@$(MAKE) $(AM_MAKEFLAGS) -C $(top_builddir)/src libs

TEST_EXTENSIONS = .tests
TESTS_LOG_COMPILER = $(srcdir)/../../run-repack-tests.py $(top_builddir)/util/hb-subset$(EXEEXT)
include Makefile.sources


-include $(top_srcdir)/git.mk
