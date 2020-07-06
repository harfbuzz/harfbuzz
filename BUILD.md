On Linux, install the development packages for FreeType,
Cairo, and GLib. For example, on Ubuntu / Debian, you would do:

    sudo apt-get install meson pkg-config ragel gtk-doc-tools gcc g++ libfreetype6-dev libglib2.0-dev libcairo2-dev

whereas on Fedora, RHEL, CentOS, and other Red Hat based systems you would do:

    sudo dnf install meson pkgconfig gtk-doc gcc gcc-c++ freetype-devel glib2-devel cairo-dev

and on ArchLinux and Manjaro:

    sudo pacman -Suy meson pkg-config ragel gcc freetype2 glib2 cairo

then use meson to build the project like `meson build && meson test -Cbuild`.

On macOS, `brew install pkg-config ragel gtk-doc freetype glib cairo meson` then use
meson like above.

On Windows, meson can build the project like above if a working MSVC's cl.exe (`vcvarsall.bat`)
or gcc/clang is already on your path, and if you use something like `meson build --wrap-mode=default`
it fetches and compiles most of the dependencies also.

There is also amalgam source provided with HarfBuzz which reduces whole process of building
HarfBuzz like `g++ src/harfbuzz.cc -fno-exceptions` but there is not guarantee provided
with buildability and reliability of features you get.

Our CI is also good source of learning how to build HarfBuzz.

Linux packagers are advised to at least use `--buildtype=release` (or any other way
to enable regular compiler optimization) and `-Dauto_features=enabled --wrap-mode=nodownload`
and install any other needed packages (most distributions build harfbuzz with
graphite support which needs to be enabled separately, `-Dgraphite=enabled`),
and follow other best practices of packaging a meson project.

Examples of meson built harfbuzz packages,
* https://git.archlinux.org/svntogit/packages.git/tree/trunk?h=packages/harfbuzz
  Which uses https://git.archlinux.org/svntogit/packages.git/tree/trunk/arch-meson?h=packages/meson
* https://git.alpinelinux.org/aports/tree/main/harfbuzz
  Which uses https://git.alpinelinux.org/aports/tree/main/meson/abuild-meson
* https://github.com/msys2/MINGW-packages/blob/master/mingw-w64-harfbuzz/PKGBUILD
* (feel free to add your distribution's source link here)
