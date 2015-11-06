# Process this file with automake to produce Makefile.in

SUBDIRS = api shaping fuzzing

# Convenience targets:
lib:
	@$(MAKE) $(AM_MAKEFLAGS) -C $(top_builddir)/src lib

-include $(top_srcdir)/git.mk
