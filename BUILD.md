# Building HarfBuzz

HarfBuzz primarily uses the _meson_ build system. A community-maintained
_CMake_ build system is also available. Finally, you can build HarfBuzz
without a build system by compiling the amalgamated source file.

## Installing dependencies

First, we need to install desired dependencies.  None of these are strictly
required to build the library, but when they are present, you can build the
library with additional _integration_ features, such as _FreeType_ integration.

The command-line tools (`hb-view`, `hb-shape`, `hb-info`, `hb-subset`) require
_GLib_, while `hb-view` also requires _Cairo_.  Most of the test suite requires
_GLib_ to run.

On Linux, install the development packages for FreeType, Cairo, and GLib. For
example, on Ubuntu / Debian, you would do:

    $ sudo apt-get install meson pkg-config ragel gtk-doc-tools gcc g++ libfreetype6-dev libglib2.0-dev libcairo2-dev

whereas on Fedora, RHEL, CentOS, and other Red Hat based systems you would do:

    $ sudo dnf install meson pkgconfig gtk-doc gcc gcc-c++ freetype-devel glib2-devel cairo-devel

and on ArchLinux and Manjaro:

    $ sudo pacman -Suy meson pkg-config ragel gcc freetype2 glib2 glib2-devel cairo

On macOS:

    brew install pkg-config ragel gtk-doc freetype glib cairo meson

## Building HarfBuzz with meson

Then use meson to build the project and run the tests, like:

    meson build && ninja -Cbuild && meson test -Cbuild

On Windows, meson can build the project like above if a working MSVC's cl.exe
(`vcvarsall.bat`) or gcc/clang is already on your path, and if you use
something like `meson build --wrap-mode=default` it fetches and compiles most
of the dependencies also.  It is recommended to install CMake either manually
or via the Visual Studio installer when building with MSVC, using meson.

Our CI configurations are also a good source of learning how to build HarfBuzz.

## Building without a build system

There is also amalgamated source provided with HarfBuzz which reduces whole process
of building HarfBuzz to `c++ -c src/harfbuzz.cc` but there is no guarantee provided
with buildability and reliability of features you get.  This can be useful when
embedding HarfBuzz in your project.

For example, the `hb-shape` tool can be built simply as:

    $ c++ -O2 src/harfbuzz.cc util/hb-shape.cc  -o hb-shape -I src -DHB_NO_FEATURES_H `pkg-config --cflags --libs glib-2.0`

See `CONFIG.md` for ideas on how to shrink the feature set of your custom built
HarfBuzz library.
