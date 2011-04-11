include Makefile.decl

if HAVE_GLIB
AM_CPPFLAGS = $(GLIB_CFLAGS) -I$(top_srcdir)/src/
LDADD = $(GLIB_LIBS) $(top_builddir)/src/libharfbuzz.la

check_PROGRAMS = $(TEST_PROGS)

TEST_PROGS += test-types


else
check-am:
	@echo "You need to have glib support enabled to run the tests"
	@exit 77
endif

-include $(top_srcdir)/git.mk
