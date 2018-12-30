# Process this file with automake to produce Makefile.in

NULL =
EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =

# Convenience targets:
lib:
	@$(MAKE) $(AM_MAKEFLAGS) -C $(top_builddir)/src lib
libs:
	@$(MAKE) $(AM_MAKEFLAGS) -C $(top_builddir)/src libs

EXTRA_DIST += CMakeLists.txt

EXTRA_DIST += fonts

LINK = $(CXXLINK)

if HAVE_GLIB
AM_CPPFLAGS = -DSRCDIR="\"$(srcdir)\"" -I$(top_srcdir)/src/ -I$(top_builddir)/src/ $(GLIB_CFLAGS)
LDADD = $(top_builddir)/src/libharfbuzz.la $(GLIB_LIBS)

EXTRA_DIST += hb-test.h hb-subset-test.h

check_PROGRAMS = $(TEST_PROGS)
noinst_PROGRAMS = $(TEST_PROGS)

TEST_PROGS = \
	test-aat-layout \
	test-baseline \
	test-blob \
	test-buffer \
	test-collect-unicodes \
	test-common \
	test-font \
	test-map \
	test-object \
	test-ot-face \
	test-set \
	test-shape \
	test-subset \
	test-subset-cmap \
	test-subset-glyf \
	test-subset-hdmx \
	test-subset-hmtx \
	test-subset-os2 \
	test-subset-post \
	test-subset-vmtx \
	test-subset-cff1 \
	test-subset-cff2 \
	test-unicode \
	test-version \
	$(NULL)

test_subset_LDADD = $(LDADD) $(top_builddir)/src/libharfbuzz-subset.la
test_subset_cmap_LDADD = $(LDADD) $(top_builddir)/src/libharfbuzz-subset.la
test_subset_glyf_LDADD = $(LDADD) $(top_builddir)/src/libharfbuzz-subset.la
test_subset_hdmx_LDADD = $(LDADD) $(top_builddir)/src/libharfbuzz-subset.la
test_subset_hmtx_LDADD = $(LDADD) $(top_builddir)/src/libharfbuzz-subset.la
test_subset_os2_LDADD  = $(LDADD) $(top_builddir)/src/libharfbuzz-subset.la
test_subset_post_LDADD = $(LDADD) $(top_builddir)/src/libharfbuzz-subset.la
test_subset_vmtx_LDADD = $(LDADD) $(top_builddir)/src/libharfbuzz-subset.la
test_subset_cff1_LDADD = $(LDADD) $(top_builddir)/src/libharfbuzz-subset.la
test_subset_cff2_LDADD = $(LDADD) $(top_builddir)/src/libharfbuzz-subset.la

test_unicode_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(GLIB_CFLAGS) \
	$(NULL)
test_unicode_LDADD = $(LDADD)
if HAVE_ICU
test_unicode_CPPFLAGS += $(ICU_CFLAGS)
test_unicode_LDADD += $(top_builddir)/src/libharfbuzz-icu.la $(ICU_LIBS)
endif


TEST_PROGS += \
	test-ot-color \
	test-ot-ligature-carets \
	test-ot-name \
	test-ot-tag \
	test-ot-extents-cff \
	$(NULL)


if HAVE_PTHREAD
if HAVE_FREETYPE
TEST_PROGS += test-multithread
test_multithread_CFLAGS = $(CFLAGS) $(PTHREAD_CFLAGS) $(FREETYPE_CFLAGS)
test_multithread_LDADD = $(LDADD) $(PTHREAD_LIBS) $(FREETYPE_LIBS)
# The auto-generated link rule somehow includes CFLAGS as well.  Without
# it, pthread link fails, because, who knows why, $PTHREAD_LIBS is empty.
test_multithread_LINK = $(LINK) $(PTHREAD_CFLAGS)
endif
endif

if HAVE_FREETYPE
TEST_PROGS += \
	test-ot-math \
	$(NULL)
test_ot_math_LDADD = $(LDADD) $(FREETYPE_LIBS)
test_ot_math_CPPFLAGS = $(AM_CPPFLAGS) $(FREETYPE_CFLAGS)
endif # HAVE_FREETYPE


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
endif

# Default test running environment
TESTS = $(TEST_PROGS)
TESTS_ENVIRONMENT = \
	MALLOC_CHECK_=2 \
	MALLOC_PERTURB_=$$(($${RANDOM:-256} % 256)) \
	G_DEBUG=gc-friendly \
	G_SLICE=always-malloc \
	srcdir=$(srcdir) \
	G_TEST_SRCDIR=$(abs_srcdir) \
	G_TEST_BUILDDIR=$(abs_builddir) \
	$(NULL)


# check-tool: Run tests under $(TOOL)
check-tool:
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) check \
	TESTS_ENVIRONMENT='$(TESTS_ENVIRONMENT) $(top_builddir)/libtool --mode=execute \
	env $(TOOL)'
# check-tool-raw: Run tests under $(TOOL), but don't run under libtool
check-tool-raw:
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) check \
	TESTS_ENVIRONMENT='$(TESTS_ENVIRONMENT) \
	env $(TOOL)'

# check-gtester: Run tests under gtester
GTESTER = gtester
check-gtester:
	$(AM_V_at)$(MAKE) $(AM_MAKEFLGS) check-tool-raw TOOL="$(GTESTER) --verbose --keep-going"


# Check tests under valgrind.  Saves log to log-valgrind.txt
VALGRIND_FLAGS = \
	--tool=memcheck \
	--track-origins=yes \
	--leak-check=yes \
	$(EXTRA_VALGRIND_FLAGS)
#	Can't do for now: --show-reachable=yes
CLEANFILES +=  log-valgrind.txt
valgrind_verbose = $(valgrind_verbose_@AM_V@)
valgrind_verbose_ = $(valgrind_verbose_@AM_DEFAULT_V@)
valgrind_verbose_0 = | \
	grep '\(^[^=]\|ERROR SUMMARY\|definitely lost\|indirectly lost\)' | grep -v ': 0'
# TODO: The following check does not fail if valgrind finds error.  It should.
check-valgrind:
	$(AM_V_at)$(MAKE) $(AM_MAKEFLGS) check-tool TOOL="valgrind $(VALGRIND_FLAGS)" \
	2>&1 | tee log-valgrind.txt $(valgrind_verbose)


# check-symbols: Finds untested API symbols
symbols-tested.txt: $(TEST_PROGS)
	$(AM_V_GEN)$(top_builddir)/libtool --mode=execute nm $^ \
	| grep ' U hb_' | sed 's/.* U hb_/hb_/' \
	| sort | uniq > $@.tmp && mv $@.tmp $@
symbols-tested-or-deprecated.txt: symbols-tested.txt $(top_builddir)/src/harfbuzz-deprecated-symbols.txt
	$(AM_V_GEN)cat $^ | sort | uniq > $@.tmp; mv $@.tmp $@
symbols-exported.txt: $(top_builddir)/src/.libs/libharfbuzz.so
	$(AM_V_GEN)$(top_builddir)/libtool --mode=execute nm $^ \
	| grep ' T ' | sed 's/.* T //' | grep -v '^\(_init\|_fini\)$$' \
	| sort | uniq > $@.tmp && mv $@.tmp $@
symbols-untested.txt: symbols-tested-or-deprecated.txt symbols-exported.txt
	$(AM_V_GEN)diff $^ > $@.tmp; mv $@.tmp $@
CLEANFILES += symbols-tested.txt \
	symbols-exported.txt \
	symbols-untested.txt \
	symbols-tested-or-deprecated.txt
check-symbols: symbols-untested.txt
	@! cat $^ | grep .



else # !HAVE_GLIB
check-am: err-glib
err-glib:
	@echo "You need to have glib support enabled to run the tests"
	@exit 77
endif # HAVE_GLIB

.PHONY: check-symbols check-tool check-valgrind

-include $(top_srcdir)/git.mk
