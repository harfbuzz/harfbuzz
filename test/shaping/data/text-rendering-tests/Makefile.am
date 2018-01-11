# Process this file with automake to produce Makefile.in

NULL =

# Convenience targets:
lib:
	@$(MAKE) $(AM_MAKEFLAGS) -C $(top_builddir)/src lib

EXTRA_DIST = \
	extract-tests.py \
	fonts \
	$(TESTS) \
	$(NULL)

TEST_EXTENSIONS = .tests
TESTS_LOG_COMPILER = $(srcdir)/../../run-tests.py $(top_builddir)/util/hb-shape$(EXEEXT)

TESTS = \
	tests/AVAR-1.tests \
	tests/CFF-1.tests \
	tests/CFF2-1.tests \
	tests/CFF-2.tests \
	tests/CMAP-1.tests \
	tests/CMAP-2.tests \
	tests/CVAR-1.tests \
	tests/CVAR-2.tests \
	tests/GLYF-1.tests \
	tests/GPOS-1.tests \
	tests/GPOS-2.tests \
	tests/GPOS-3.tests \
	tests/GPOS-4.tests \
	tests/GPOS-5.tests \
	tests/GSUB-1.tests \
	tests/GSUB-2.tests \
	tests/GVAR-1.tests \
	tests/GVAR-2.tests \
	tests/GVAR-3.tests \
	tests/GVAR-4.tests \
	tests/GVAR-5.tests \
	tests/GVAR-6.tests \
	tests/GVAR-7.tests \
	tests/GVAR-8.tests \
	tests/GVAR-9.tests \
	tests/HVAR-1.tests \
	tests/HVAR-2.tests \
	tests/KERN-1.tests \
	tests/KERN-2.tests \
	tests/SHBALI-3.tests \
	tests/SHKNDA-1.tests \
	$(NULL)
DISABLED_TESTS = \
	tests/CMAP-3.tests # Non-Unicode cmap \
	tests/MORX-1.tests # Not implemented \
	tests/MORX-2.tests # Not implemented \
	tests/MORX-3.tests # Not implemented \
	tests/MORX-4.tests # Not implemented \
	tests/SHARAN-1.tests # Rounding difference \
	tests/SHBALI-1.tests # Rounding difference \
	tests/SHBALI-2.tests # Rounding difference \
	tests/SHKNDA-2.tests # Rounding difference \
	tests/SHKNDA-3.tests # Rounding difference \
	$(NULL)

-include $(top_srcdir)/git.mk
