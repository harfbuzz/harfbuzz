#!/usr/bin/env python3

"This tool is intended to be used from meson"

import os, os.path, sys, subprocess, shutil

ragel = sys.argv[1]
if not ragel:
	sys.exit ('You have to install ragel if you are going to develop HarfBuzz itself')

if len (sys.argv) < 4:
	sys.exit (__doc__)

OUTPUT = sys.argv[2]
CURRENT_SOURCE_DIR = sys.argv[3]
INPUT = sys.argv[4]

outdir = os.path.dirname (OUTPUT)
shutil.copy (INPUT, outdir)
rl = os.path.basename (INPUT)
hh = rl.replace ('.rl', '.hh')
ret = subprocess.Popen (ragel.split() + ['-e', '-F1', '-o', hh, rl], cwd=outdir).wait ()
if ret:
    sys.exit (ret)

# copy it also to src/; if read-only, verify committed copy matches.
generated = os.path.join (outdir, hh)
src_copy = os.path.join (CURRENT_SOURCE_DIR, hh)
try:
    shutil.copyfile (generated, src_copy)
except OSError:
    import filecmp
    if not filecmp.cmp (generated, src_copy, shallow=False):
        sys.exit (f'{src_copy} is out of date; regenerate with a writable source tree')
