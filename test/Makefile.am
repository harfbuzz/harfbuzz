include Makefile.decl

NULL =

if HAVE_GLIB
AM_CPPFLAGS = -I$(top_srcdir)/src/ $(GLIB_CFLAGS)
LDADD = $(top_builddir)/src/libharfbuzz.la $(GLIB_LIBS)

check_PROGRAMS = $(TEST_PROGS)
noinst_PROGRAMS = $(TEST_PROGS)

TEST_PROGS += \
	test-buffer \
	test-common \
	test-object \
	test-unicode \
	$(NULL)


# Tests for header compilation
TEST_PROGS += \
	test-c \
	test-cplusplus \
	$(NULL)
test_cplusplus_SOURCES = test-cplusplus.cc
test_c_CPPFLAGS = $(AM_CPPFLAGS)
test_cplusplus_CPPFLAGS = $(AM_CPPFLAGS)
if HAVE_ICU
test_c_CPPFLAGS += $(ICU_CFLAGS)
test_cplusplus_CPPFLAGS += $(ICU_CFLAGS)
endif
if HAVE_FREETYPE
test_c_CPPFLAGS += $(FREETYPE_CFLAGS)
test_cplusplus_CPPFLAGS += $(FREETYPE_CFLAGS)
# TODO replace freetype with other stuff in the following test
test_object_CPPFLAGS = $(AM_CPPFLAGS) $(FREETYPE_CFLAGS)
test_object_LIBS = $(LDADD) $(FREETYPE_LIBS)
endif


else
check-am:
	@echo "You need to have glib support enabled to run the tests"
	@exit 77
endif

-include $(top_srcdir)/git.mk
