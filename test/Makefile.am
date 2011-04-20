include Makefile.decl

NULL =

if HAVE_GLIB
AM_CPPFLAGS = -I$(top_srcdir)/src/ $(GLIB_CFLAGS)
LDADD = $(top_builddir)/src/libharfbuzz.la $(GLIB_LIBS)

check_PROGRAMS = $(TEST_PROGS)

TEST_PROGS += \
	test-buffer \
	test-common \
	test-unicode \
	$(NULL)



else
check-am:
	@echo "You need to have glib support enabled to run the tests"
	@exit 77
endif

-include $(top_srcdir)/git.mk
