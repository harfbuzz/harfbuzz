AC_PREREQ([2.64])
AC_INIT([harfbuzz],
        [0.5.0],
        [http://bugs.freedesktop.org/enter_bug.cgi?product=harfbuzz],
        [harfbuzz],
        [http://harfbuzz.org/])

AC_CONFIG_SRCDIR([harfbuzz.pc.in])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11.1 gnu dist-bzip2 no-dist-gzip -Wall no-define])
AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

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

# Functions and headers
AC_CHECK_FUNCS(mprotect sysconf getpagesize mmap)
AC_CHECK_HEADERS(unistd.h sys/mman.h)

# Compiler flags
AC_CANONICAL_HOST
if test "x$GCC" = "xyes"; then

	# Make symbols link locally
	LDFLAGS="$LDFLAGS -Bsymbolic-functions"

	# Make sure we don't link to libstdc++
	CXXFLAGS="$CXXFLAGS -fno-rtti -fno-exceptions"

	case "$host" in
		arm-*-*)
			# Request byte alignment on arm
			CXXFLAGS="$CXXFLAGS -mstructure-size-boundary=8"
		;;
	esac
fi

dnl ==========================================================================

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.16, have_glib=true, have_glib=false)
PKG_CHECK_MODULES(GTHREAD, gthread-2.0, , have_glib=false)
if $have_glib; then
	AC_DEFINE(HAVE_GLIB, 1, [Have glib2 library])
fi
AM_CONDITIONAL(HAVE_GLIB, $have_glib)

dnl ==========================================================================

PKG_CHECK_MODULES(CAIRO, cairo >= 1.8.0, have_cairo=true, have_cairo=false)
if $have_cairo; then
	AC_DEFINE(HAVE_CAIRO, 1, [Have cairo graphics library])
fi
AM_CONDITIONAL(HAVE_CAIRO, $have_cairo)

PKG_CHECK_MODULES(CAIRO_PNG, cairo-png, have_cairo_png=true, have_cairo_png=false)
if $have_cairo_png; then
	AC_DEFINE(HAVE_CAIRO_PNG, 1, [Have cairo-png support in cairo graphics library])
fi
AM_CONDITIONAL(HAVE_CAIRO_PNG, $have_cairo_png)

PKG_CHECK_MODULES(CAIRO_FT, cairo-ft, have_cairo_ft=true, have_cairo_ft=false)
if $have_cairo_ft; then
	AC_DEFINE(HAVE_CAIRO_FT, 1, [Have cairo-ft support in cairo graphics library])
fi
AM_CONDITIONAL(HAVE_CAIRO_FT, $have_cairo_ft)

dnl ==========================================================================

PKG_CHECK_MODULES(ICU, icu, have_icu=true, [
	AC_CHECK_PROG([have_icu], [icu-config], [true], [false])
	if $have_icu; then
		icu_cflags=`icu-config --cppflags`
		icu_libs=`icu-config --ldflags-libsonly`
		AC_SUBST(ICU_CFLAGS, [$icu_cflags])
		AC_SUBST(ICU_LIBS, [$icu_libs])
	fi
])
if $have_icu; then
	AC_DEFINE(HAVE_ICU, 1, [Have ICU library])
fi
AM_CONDITIONAL(HAVE_ICU, $have_icu)

dnl ==========================================================================

PKG_CHECK_MODULES(FREETYPE, freetype2, have_freetype=true, have_freetype=false)
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

AC_CONFIG_FILES([
Makefile
harfbuzz.pc
src/Makefile
src/hb-version.h
test/Makefile
])

AC_OUTPUT
