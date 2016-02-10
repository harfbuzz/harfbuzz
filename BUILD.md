On Linux, install the development packages for
[FreeType](http://www.freedesktop.org/wiki/Software/FreeType/),
Cairo, and GLib. For example, on Ubuntu / Debian, you would do:
* sudo apt-get install gcc g++ libfreetype6-dev libglib2.0-dev libcairo2-dev

whereas on Fedora, RHEL, CentOS, and other Red Hat based systems you would do:
* sudo yum install gcc gcc-c++ freetype-devel glib2-devel cairo-devel

on the Mac, using MacPorts:
* sudo port install freetype glib2 cairo

or using Homebrew:
* brew install freetype glib cairo

If you are using a tarball, you can now proceed to running configure and make
as with any other standard package. That should leave you with a shared
library in src/, and a few utility programs including hb-view and hb-shape
under util/.  From the tarball, NMake Makefiles are also provided in win32/,
which supports building HarfBuzz using Visual Studio, with a README.txt that
gives instructions on building using NMake.
If you are bootstraping from git, you need a few more tools before you can
run autogen.sh for the first time. Namely, pkg-config and ragel. Again,
on Ubuntu / Debian:
* sudo apt-get install autoconf automake libtool pkg-config ragel gtk-doc-tools

and on Fedora, RHEL, CentOS:
* sudo yum install autoconf automake libtool pkgconfig ragel gtk-doc

on the Mac, using MacPorts:
* sudo port install autoconf automake libtool pkgconfig ragel gtk-doc

or using Homebrew:
* brew port install autoconf automake libtool pkgconfig ragel gtk-doc
