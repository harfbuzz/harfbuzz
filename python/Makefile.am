# Process this file with automake to produce Makefile.in

NULL =
EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =

EXTRA_DIST += \
	hb \
	$(NULL)

-include $(top_srcdir)/git.mk

if HAVE_PYTHON
bin_SCRIPTS = hb

dist_pkgpython_PYTHON = \
	__init__.py \
	tools.py
endif
