#!/usr/bin/python
# vim: encoding=utf-8
#expand *.in files
#this script is only intended for building from git, not for building from the released tarball, which already includes all necessary files
import os
import sys
import re
import string
import subprocess
import optparse
from pc_base import BasePCItems
from replace import replace_multi

def get_version_items(srcroot):
    ver = {}
    RE_VERSION_LINE_START = re.compile(r'^AC_INIT\(\[(.+)\], *\n')
    RE_VERSION_LINE_BODY = re.compile(r'^        \[(.+)\], *\n')
    RE_VERSION_LINE_END = re.compile(r'^        \[(.+)\]\) *\n')

    # Read from the AC_INIT lines to get the version/name/URLs info
    with open(os.path.join(srcroot, 'configure.ac'), 'r') as ac:
        for i in ac:
            mo_init = RE_VERSION_LINE_START.search(i)
            mo_pkg_info = RE_VERSION_LINE_BODY.search(i)
            mo_pkg_url = RE_VERSION_LINE_END.search(i)
            if mo_init:
                ver['@PACKAGE_NAME@'] = mo_init.group(1) 
            if mo_pkg_info:
                if mo_pkg_info.group(1).startswith('http'):
                    ver['@PACKAGE_BUGREPORT@'] = mo_pkg_info.group(1)
                elif mo_pkg_info.group(1)[0].isdigit():
                    ver['@PACKAGE_VERSION@'] = mo_pkg_info.group(1)
                else:
                    ver['@PACKAGE_TARNAME@'] = mo_pkg_info.group(1)
            if mo_pkg_url:
                ver['@PACKAGE_URL@'] = mo_pkg_url.group(1)
            
    ver['@HB_VERSION@'] = ver['@PACKAGE_VERSION@']

    pkg_ver_parts = ver['@PACKAGE_VERSION@'].split('.')
    ver['@HB_VERSION_MAJOR@'] = pkg_ver_parts[0]
    ver['@HB_VERSION_MINOR@'] = pkg_ver_parts[1]
    ver['@HB_VERSION_MICRO@'] = pkg_ver_parts[2]
    return ver

def main(argv):
    pc = BasePCItems()
    srcroot = pc.top_srcdir
    srcdir = pc.srcdir
    ver = get_version_items(srcroot)

    replace_multi(os.path.join(srcdir, 'config.h.win32.in'),
                  os.path.join(srcdir, 'config.h.win32'),
                  ver)

    replace_multi(os.path.join(srcroot, 'src', 'hb-version.h.in'),
                  os.path.join(srcroot, 'src', 'hb-version.h'),
                  ver)
    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))
