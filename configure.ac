AC_PREREQ([2.64])
AC_INIT([HarfBuzz],
        [0.9.10],
        [http://bugs.freedesktop.org/enter_bug.cgi?product=harfbuzz],
        [harfbuzz],
        [http://harfbuzz.org/])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([harfbuzz.pc.in])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11.1 gnits dist-bzip2 no-dist-gzip -Wall no-define])
AM_SILENT_RULES([yes])

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
PKG_PROG_PKG_CONFIG([0.20])

# Version
m4_define(hb_version_triplet,m4_split(AC_PACKAGE_VERSION,[[.]]))
m4_define(hb_version_major,m4_argn(1,hb_version_triplet))
m4_define(hb_version_minor,m4_argn(2,hb_version_triplet))
m4_define(hb_version_micro,m4_argn(3,hb_version_triplet))
HB_VERSION_MAJOR=hb_version_major
HB_VERSION_MINOR=hb_version_minor
HB_VERSION_MICRO=hb_version_micro
HB_VERSION=AC_PACKAGE_VERSION
AC_SUBST(HB_VERSION_MAJOR)
AC_SUBST(HB_VERSION_MINOR)
AC_SUBST(HB_VERSION_MICRO)
AC_SUBST(HB_VERSION)

# Libtool version
m4_define([hb_version_int],
	  m4_eval(hb_version_major*10000 + hb_version_minor*100 + hb_version_micro))
m4_if(m4_eval(hb_version_minor % 2), [1],
      dnl for unstable releases
      [m4_define([hb_libtool_revision], 0)],
      dnl for stable releases
      [m4_define([hb_libtool_revision], hb_version_micro)])
m4_define([hb_libtool_age],
	  m4_eval(hb_version_int - hb_libtool_revision))
m4_define([hb_libtool_current],
	  m4_eval(hb_version_major + hb_libtool_age))
HB_LIBTOOL_VERSION_INFO=hb_libtool_current:hb_libtool_revision:hb_libtool_age
AC_SUBST(HB_LIBTOOL_VERSION_INFO)

dnl GOBJECT_INTROSPECTION_CHECK([0.9.0])
dnl GTK_DOC_CHECK([1.15],[--flavour no-tmpl])

# Functions and headers
AC_CHECK_FUNCS(atexit mprotect sysconf getpagesize sched_yield mmap _setmode isatty)
AC_CHECK_HEADERS(unistd.h sys/mman.h sched.h io.h)

# Compiler flags
AC_CANONICAL_HOST
if test "x$GCC" = "xyes"; then

	# Make symbols link locally
	LDFLAGS="$LDFLAGS -Bsymbolic-functions"

	# Make sure we don't link to libstdc++
	CXXFLAGS="$CXXFLAGS -fno-rtti -fno-exceptions"

	# Assorted warnings
	CXXFLAGS="$CXXFLAGS -Wcast-align"

	case "$host" in
		*-*-mingw*)
		;;
		*)
			# Hide inline methods
			CXXFLAGS="$CXXFLAGS -fvisibility-inlines-hidden"
		;;
	esac

	case "$host" in
		arm-*-*)
			# Request byte alignment on arm
			CXXFLAGS="$CXXFLAGS -mstructure-size-boundary=8"
		;;
	esac
fi

hb_os_win32=no
AC_MSG_CHECKING([for native Win32])
case "$host" in
  *-*-mingw*)
    hb_os_win32=yes
    ;;
esac
AC_MSG_RESULT([$hb_os_win32])
AM_CONDITIONAL(OS_WIN32, test "$hb_os_win32" = "yes")

have_pthread=false
if test "$hb_os_win32" = no; then
	AX_PTHREAD([have_pthread=true])
fi
if $have_pthread; then
	AC_DEFINE(HAVE_PTHREAD, 1, [Have POSIX threads])
fi
AM_CONDITIONAL(HAVE_PTHREAD, $have_pthread)

dnl ==========================================================================

have_ot=true
if $have_ot; then
	AC_DEFINE(HAVE_OT, 1, [Have native OpenType Layout backend])
fi
AM_CONDITIONAL(HAVE_OT, $have_ot)

dnl ===========================================================================

have_hb_old=true
if $have_hb_old; then
	AC_DEFINE(HAVE_HB_OLD, 1, [Have Old HarfBuzz backend])
fi
AM_CONDITIONAL(HAVE_HB_OLD, $have_hb_old)

dnl ===========================================================================

have_ucdn=true
if $have_ucdn; then
	AC_DEFINE(HAVE_UCDN, 1, [Have UCDN Unicode functions])
fi
AM_CONDITIONAL(HAVE_UCDN, $have_ucdn)

dnl ===========================================================================

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.16, have_glib=true, have_glib=false)
if $have_glib; then
	AC_DEFINE(HAVE_GLIB, 1, [Have glib2 library])
fi
AM_CONDITIONAL(HAVE_GLIB, $have_glib)

PKG_CHECK_MODULES(GTHREAD, gthread-2.0, have_gthread=true, have_gthread=false)
if $have_gthread; then
	AC_DEFINE(HAVE_GTHREAD, 1, [Have gthread2 library])
fi
AM_CONDITIONAL(HAVE_GTHREAD, $have_gthread)

PKG_CHECK_MODULES(GOBJECT, gobject-2.0 glib-2.0 >= 2.16, have_gobject=true, have_gobject=false)
if $have_gobject; then
	AC_DEFINE(HAVE_GOBJECT, 1, [Have gobject2 library])
	GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`
	AC_SUBST(GLIB_MKENUMS)
fi
AM_CONDITIONAL(HAVE_GOBJECT, $have_gobject)

dnl ==========================================================================

PKG_CHECK_MODULES(CAIRO, cairo >= 1.8.0, have_cairo=true, have_cairo=false)
if $have_cairo; then
	AC_DEFINE(HAVE_CAIRO, 1, [Have cairo graphics library])
fi
AM_CONDITIONAL(HAVE_CAIRO, $have_cairo)

PKG_CHECK_MODULES(CAIRO_FT, cairo-ft, have_cairo_ft=true, have_cairo_ft=false)
if $have_cairo_ft; then
	AC_DEFINE(HAVE_CAIRO_FT, 1, [Have cairo-ft support in cairo graphics library])
fi
AM_CONDITIONAL(HAVE_CAIRO_FT, $have_cairo_ft)

dnl ==========================================================================

PKG_CHECK_MODULES(ICU, icu-uc, have_icu=true, have_icu=false)
if $have_icu; then
	AC_DEFINE(HAVE_ICU, 1, [Have ICU library])
fi
AM_CONDITIONAL(HAVE_ICU, $have_icu)

dnl ==========================================================================

PKG_CHECK_MODULES(ICU_LE, icu-le icu-uc, have_icu_le=true, have_icu_le=false)
if $have_icu_le; then
	AC_DEFINE(HAVE_ICU_LE, 1, [Have ICU Layout Engine library])
fi
AM_CONDITIONAL(HAVE_ICU_LE, $have_icu_le)

dnl ==========================================================================

PKG_CHECK_MODULES(GRAPHITE2, graphite2, have_graphite=true, have_graphite=false)
if $have_graphite; then
    AC_DEFINE(HAVE_GRAPHITE2, 1, [Have Graphite library])
fi
AM_CONDITIONAL(HAVE_GRAPHITE2, $have_graphite)

dnl ==========================================================================

PKG_CHECK_MODULES(FREETYPE, freetype2 >= 2.3.8, have_freetype=true, have_freetype=false)
if $have_freetype; then
	AC_DEFINE(HAVE_FREETYPE, 1, [Have FreeType 2 library])
	_save_libs="$LIBS"
	_save_cflags="$CFLAGS"
	LIBS="$LIBS $FREETYPE_LIBS"
	CFLAGS="$CFLAGS $FREETYPE_CFLAGS"
	AC_CHECK_FUNCS(FT_Face_GetCharVariantIndex)
	LIBS="$_save_libs"
	CFLAGS="$_save_cflags"
fi
AM_CONDITIONAL(HAVE_FREETYPE, $have_freetype)

dnl ===========================================================================

AC_MSG_CHECKING([for ScriptShapeOpenType in usp10])
saved_LIBS=$LIBS
LIBS="$LIBS -lusp10 -lgdi32"
AC_LINK_IFELSE([AC_LANG_PROGRAM(
	[[
	 #define _WIN32_WINNT 0x0600
	 #include <windows.h>
	 #include <usp10.h>
	 ]],
	ScriptShapeOpenType)],
	[have_uniscribe=true; AC_MSG_RESULT(yes)],
	[have_uniscribe=false;AC_MSG_RESULT(no)])
LIBS=$saved_LIBS
if $have_uniscribe; then
	UNISCRIBE_CFLAGS=
	UNISCRIBE_LIBS="-lusp10 -lgdi32"
	AC_SUBST(UNISCRIBE_CFLAGS)
	AC_SUBST(UNISCRIBE_LIBS)
	AC_DEFINE(HAVE_UNISCRIBE, 1, [Have Uniscribe backend])
fi
AM_CONDITIONAL(HAVE_UNISCRIBE, $have_uniscribe)

dnl ===========================================================================

AC_CHECK_TYPE(CTFontRef, have_coretext=true, have_coretext=false, [#include <ApplicationServices/ApplicationServices.h>])
if $have_coretext; then
	CORETEXT_CFLAGS=
	CORETEXT_LIBS="-framework ApplicationServices"
	AC_SUBST(CORETEXT_CFLAGS)
	AC_SUBST(CORETEXT_LIBS)
	AC_DEFINE(HAVE_CORETEXT, 1, [Have Core Text backend])
fi
AM_CONDITIONAL(HAVE_CORETEXT, $have_coretext)

dnl ===========================================================================

AC_CACHE_CHECK([for Intel atomic primitives], hb_cv_have_intel_atomic_primitives, [
	hb_cv_have_intel_atomic_primitives=false
	AC_TRY_LINK([
		void memory_barrier (void) { __sync_synchronize (); }
		int atomic_add (int *i) { return __sync_fetch_and_add (i, 1); }
		int mutex_trylock (int *m) { return __sync_lock_test_and_set (m, 1); }
		void mutex_unlock (int *m) { __sync_lock_release (m); }
		], [], hb_cv_have_intel_atomic_primitives=true
	)
])
if $hb_cv_have_intel_atomic_primitives; then
	AC_DEFINE(HAVE_INTEL_ATOMIC_PRIMITIVES, 1, [Have Intel __sync_* atomic primitives])
fi

dnl ===========================================================================

AC_CONFIG_FILES([
Makefile
harfbuzz.pc
src/Makefile
src/hb-version.h
src/hb-icu-le/Makefile
src/hb-old/Makefile
src/hb-ucdn/Makefile
util/Makefile
test/Makefile
test/api/Makefile
test/shaping/Makefile
])

AC_OUTPUT

AC_MSG_NOTICE([

Build configuration:

Unicode callbacks (you want at least one):
	Glib:			${have_glib}
	ICU:			${have_icu}
	UCDN:			${have_ucdn}

Font callbacks (the more the better):
	FreeType:		${have_freetype}

Tools used for command-line utilities:
	Cairo:			${have_cairo}

Additional shapers (the more the better):
	Graphite2:		${have_graphite}

Test / platform shapers (not normally needed):
	CoreText:		${have_coretext}
	ICU Layout Engine:	${have_icu_le}
	Old HarfBuzz:		${have_hb_old}
	Uniscribe:		${have_uniscribe}
])
