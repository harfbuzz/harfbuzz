include Makefile.decl

NULL =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =

if HAVE_GLIB
AM_CPPFLAGS = -I$(top_srcdir)/src/ $(GLIB_CFLAGS) $(GTHREAD_CFLAGS)
LDADD = $(top_builddir)/src/libharfbuzz.la $(GLIB_LIBS) $(GTHREAD_LIBS)

EXTRA_DIST += hb-test.h

check_PROGRAMS = $(TEST_PROGS)
noinst_PROGRAMS = $(TEST_PROGS)

TEST_PROGS += \
	test-blob \
	test-buffer \
	test-common \
	test-font \
	test-object \
	test-shape \
	test-unicode \
	test-version \
	$(NULL)

TEST_PROGS += \
	test-ot-tag \
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


# test-symbols: Finds untested API symbols
symbols-tested.txt: $(TEST_PROGS)
	$(AM_V_GEN)$(top_builddir)/libtool --mode=execute nm $^ \
	| grep ' U hb_' | sed 's/.* U hb_/hb_/' \
	| sort | uniq > $@.tmp && mv $@.tmp $@
symbols-exported.txt: $(top_builddir)/src/.libs/libharfbuzz.so
	$(AM_V_GEN)$(top_builddir)/libtool --mode=execute nm $^ \
	| grep ' T ' | sed 's/.* T //' | grep -v '^\(_init\|_fini\)$$' \
	| sort | uniq > $@.tmp && mv $@.tmp $@
symbols-untested.txt: symbols-tested.txt symbols-exported.txt
	$(AM_V_GEN)diff $^ > $@.tmp; mv $@.tmp $@
CLEANFILES += symbols-tested.txt symbols-exported.txt symbols-untested.txt
test-symbols: symbols-untested.txt
	@! cat $^ | grep .


else
check-am:
	@echo "You need to have glib support enabled to run the tests"
	@exit 77
endif

.PHONY: test-symbols

-include $(top_srcdir)/git.mk
