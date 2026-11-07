#!/usr/bin/env python3

import os
import shutil
import subprocess
import sys

from check_helpers import CHECK_LIBSTDCXX_LIBS, CHECK_SYMBOL_LIBS

srcdir = sys.argv[1]
base_srcdir = sys.argv[2]
builddir = sys.argv[3]

os.chdir(srcdir)

ldd = os.getenv("LDD", shutil.which("ldd"))
readelf = os.getenv("READELF") or shutil.which("readelf") or shutil.which("llvm-readelf")
if not ldd:
    otool = os.getenv("OTOOL", shutil.which("otool"))
    if otool:
        ldd = otool + " -L"
    else:
        print("check-libstdc++.py: 'ldd' not found; skipping test")
        sys.exit(77)

stat = 0
tested = False

check_libs = CHECK_SYMBOL_LIBS
if not readelf:
    check_libs = CHECK_LIBSTDCXX_LIBS

for soname in check_libs:
    for suffix in ["so", "dylib"]:
        so = os.path.join(builddir, "lib%s.%s" % (soname, suffix))
        if not os.path.exists(so):
            continue

        print("Checking that we are not linking to libstdc++ or libc++ in %s" % so)
        if readelf:
            dynamic = subprocess.check_output([readelf, "-d", so])
            ldd_result = b"\n".join(line for line in dynamic.splitlines() if b"(NEEDED)" in line)
        else:
            ldd_result = subprocess.check_output(ldd.split() + [so])
        if (b"libstdc++" in ldd_result) or (b"libc++" in ldd_result):
            print("Ouch, %s is linked to libstdc++ or libc++" % so)
            stat = 1

        tested = True

if not tested:
    print("check-libstdc++.py: libharfbuzz shared library not found; skipping test")
    sys.exit(77)

sys.exit(stat)
