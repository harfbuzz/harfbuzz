#!/usr/bin/env python3

import os, subprocess
from pathlib import Path

outfile = Path(
    os.getenv('MESON_DIST_ROOT')
    or os.getenv('MESON_SOURCE_ROOT')
    or Path(__file__).parent
) / '.tarball-revision'

with open(outfile, 'wb') as f:
	f.write(subprocess.check_output(['git', 'log', '--no-color', '--no-decorate', 'HEAD~..HEAD']))
