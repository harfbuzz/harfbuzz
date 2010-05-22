#!/usr/bin/python

from distutils.core import setup
from glob import glob
from Pyrex.Distutils.extension import Extension
from Pyrex.Distutils import build_ext

setup(name='harfbuzz',
    version='0.0.1',
    description='Harfbuzz compatibility layer',
    long_description='Harfbuzz python integration modules and supporting scripts',
    maintainer='Martin Hosken',
    maintainer_email='martin_hosken@sil.org',
    packages=['harfbuzz'],
    ext_modules = [
        Extension("harfbuzz", ["lib/harfbuzz.pyx"], libraries=["harfbuzz"], library_dirs=["../../src/.libs"], include_dirs=["/usr/include/freetype2", "../../src"]),
        Extension("fontconfig", ["lib/fontconfig.pyx"], libraries=["fontconfig"])
        ],
    cmdclass = {'build_ext' : build_ext},
    scripts = glob('scripts/*'),
    license = 'LGPL',
    platforms = ['Linux', 'Win32', 'Mac OS X'],
    package_dir = {'harfbuzz' : 'lib'}
)

