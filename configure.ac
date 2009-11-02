AC_PREREQ(2.59)
AC_INIT(harfbuzz, 0.1, [http://bugs.freedesktop.org/enter_bug.cgi?product=harfbuzz])
AC_CONFIG_SRCDIR([harfbuzz.pc.in])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.9.6 gnu dist-bzip2 no-dist-gzip -Wall no-define])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL dnl ([1.4]) Don't remove!

AC_PROG_CC
AC_PROG_CXX

AC_CHECK_FUNCS(mprotect sysconf getpagesize)
AC_CHECK_HEADERS(unistd.h sys/mman.h)

# Make sure we don't link to libstdc++
if test "x$GCC" = "xyes"; then
	CXXFLAGS="$CXXFLAGS -fno-rtti -fno-exceptions"
fi

dnl ==========================================================================

PKG_CHECK_MODULES(GLIB, glib-2.0 gthread-2.0, have_glib=true, have_glib=false)
AC_DEFINE(HAVE_GLIB, 1, [Have FreeType 2 library])
AM_CONDITIONAL(HAVE_GLIB, $have_glib)

PKG_CHECK_MODULES(FREETYPE, freetype, have_freetype=true, have_freetype=false)
AC_DEFINE(HAVE_FREETYPE, 1, [Have FreeType 2 library])
AM_CONDITIONAL(HAVE_FREETYPE, $have_freetype)

dnl ==========================================================================

AC_CONFIG_FILES([
harfbuzz.pc
Makefile
src/Makefile
])

AC_OUTPUT
