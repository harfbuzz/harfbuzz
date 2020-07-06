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

On Windows, meson builds the project like above if a working MSVC's cl.exe (`vcvarsall.bat`)
or gcc/clang is already on your path, it fetches and compiles most of the dependencies also.

Linux packagers are advised to at least use `--buildtype=release -Dauto_features=enabled --wrap-mode=nodownload`
and install any other needed packages (most distributions build harfbuzz with
graphite support which needed to enabled separately, `-Dgraphite=enabled`),
and follow other best practices of packaging a meson project.
