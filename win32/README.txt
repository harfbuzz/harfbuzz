Instructions for building HarfBuzz on Visual Studio
===================================================
Building the HarfBuzz DLL on Windows is now also supported using Visual Studio
versions 2008 through 2015, in both 32-bit and 64-bit (x64) flavors, via NMake
Makefiles.

The following are instructions for performing such a build, as there is a
number of build configurations supported for the build.  Note that for all
build configurations, the OpenType and Simple TrueType layout (fallback)
backends are enabled, and this is the base configuration that is built if no
options (see below) are specified.  A 'clean' target is provided-it is recommended
that one cleans the build and redo the build if any configuration option changed.
An 'install' target is also provided to copy the built items in their appropriate
locations under $(PREFIX), which is described below.  A 'reallyclean' target is
provided that not only does what is done for the 'clean' target, but also removes
the sources/headers that are generated from the Ragel sources.  Therefore, if one
is not building from a release tarball, or is rebuilding after using the 'reallyclean'
target or when the Ragel (*.rl) sources are updated, the Ragel state machine
compiler (ragel.exe) is needed, and needs to be passed in via RAGEL=<path_to_ragel_exe>
if ragel.exe is not already in the PATH.

We now support building from a GIT checkout via NMake for convenience.  In addition to
the requirements as outlined in the later part of this file, you will need to run the
setup.py (Python 2.7.x or later) script to generate the headers (src\hb-version.h and
win32\config.h.win32) that are normally shipped in a release tarball before running
NMake, and you will need to pass RAGEL=<path_to_ragel_exe> if the Ragel state machine
compiler (ragel.exe) is not in your PATH when invoking NMake.  Note that the
'reallyclean' target does not remove these 2 generated headers, so re-run the setup.py
script if necessary.

Invoke the build by issuing the command:
nmake /f Makefile.vc CFG=[release|debug] [PREFIX=...] <option1=1 option2=1 ...>
where:

CFG: Required.  Choose from a release or debug build.  Note that 
     all builds generate a .pdb file for each .dll and .exe built--this refers
     to the C/C++ runtime that the build uses.

PREFIX: Optional.  Base directory of where the third-party headers, libraries
        and needed tools can be found, i.e. headers in $(PREFIX)\include,
        libraries in $(PREFIX)\lib and tools in $(PREFIX)\bin.  If not
        specified, $(PREFIX) is set as $(srcroot)\..\vs$(X)\$(platform), where
        $(platform) is win32 for 32-bit builds or x64 for 64-bit builds, and
        $(X) is the short version of the Visual Studio used, as follows:
        2008: 9
        2010: 10
        2012: 11
        2013: 12
        2015: 14
        2017: 15

Explanation of options, set by <option>=1:
------------------------------------------
GLIB: Enable GLib support in HarfBuzz, which also uses the GLib unicode
      callback if the bundled UCDN unicode callback is disabled.  This requires the
      GLib libraries, and is required for building all tool and test programs.

GOBJECT: Enable building the HarfBuzz-GObject DLL, and thus implies GLib
         support.  This requires the GObject libraries and glib-mkenums script,
         along with PERL to generate the enum sources and headers, which is
         required for the build.

INTROSPECTION: Enable build of introspection files, for making HarfBuzz
               bindings for other programming languages available, such as
               Python, available.  This requires the GObject-Introspection
               libraries and tools, along with the Python interpretor that was
               used during the build of GObject-Introspection.  Please see
               $(srcroot)\README.python for more related details.  This implies
               the build of the HarfBuzz-GObject DLL, along with GLib support.

FREETYPE: Enable the FreeType font callbacks.  Requires the FreeType2 library.

CAIRO: Enable Cairo support.  Requires the Cairo library.

CAIRO_FT: Enable the build of the hb-view tool, which makes use of Cairo, and
          thus implies FreeType font callback support and Cairo support.
          Requires Cairo libraries built with FreeType support.  Note that the
          hb-view tool requires GLib support as well.

GRAPHITE2: Enable the Graphite2 shaper, requires the SIL Graphite2 library.

ICU: Enables the build of ICU Unicode functions. Requires the ICU libraries.

UNISCRIBE: Enable Uniscribe platform shaper support.

DIRECTWRITE: Enable DirectWrite platform shaper support,
             requires a rather recent Windows SDK, and at least Windows Vista/
             Server 2008 with SP2 and platform update.

PYTHON: Full path to the Python interpretor to be used, if it is not in %PATH%.

PERL: Full path to the PERL interpretor to be used, if it is not in %PATH%.

RAGEL: Full path to the Ragel state machine compiler executable, if not in %PATH%

LIBTOOL_DLL_NAME: Enable libtool-style DLL names.
