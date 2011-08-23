#!/usr/bin/python

from distutils.core import setup
from optparse import OptionParser
from glob import glob
from Pyrex.Distutils.extension import Extension
from Pyrex.Distutils import build_ext

parser = OptionParser()
parser.add_option('-b','--build', help='Build directory in which libraries are found. Relative to project root')
parser.disable_interspersed_args()

(opts, args) = parser.parse_args()

rfile = file("runpy", "w")
rfile.write("""#!/bin/sh
LD_LIBRARY_PATH=../../%s/src/.libs PYTHONPATH=build/lib.`python -c 'import distutils.util, sys; print distutils.util.get_platform()+"-"+str(sys.version_info[0])+"."+str(sys.version_info[1])'` "$@"
""" % opts.build)
rfile.close()

setup(name='harfbuzz',
    version='0.0.1',
    description='Harfbuzz compatibility layer',
    long_description='Harfbuzz python integration modules and supporting scripts',
    maintainer='Martin Hosken',
    maintainer_email='martin_hosken@sil.org',
    packages=['harfbuzz'],
    ext_modules = [
        Extension("harfbuzz", ["lib/harfbuzz.pyx"], libraries=["harfbuzz"], library_dirs=["../../%s/src/.libs" % opts.build], include_dirs=["/usr/include/freetype2", "../../src", "../../%s/src" % opts.build]),
        Extension("fontconfig", ["lib/fontconfig.pyx"], libraries=["fontconfig"])
        ],
    cmdclass = {'build_ext' : build_ext},
    scripts = glob('scripts/*'),
    license = 'LGPL',
    platforms = ['Linux', 'Win32', 'Mac OS X'],
    package_dir = {'harfbuzz' : 'lib'},
    script_args = args
)

