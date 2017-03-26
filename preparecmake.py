#!/usr/bin/env python

import re

BLACKLIST = [
    'hb-buffer-deserialize-json.hh',
    'hb-buffer-deserialize-text.hh',
    'hb-ot-shape-complex-indic-machine.hh',
    'hb-ot-shape-complex-myanmar-machine.hh',
    'hb-ot-shape-complex-use-machine.hh'
]

makefile = open('src/Makefile.sources').read()
cmakefile = open('CMakeLists.txt.in').read()

for m in re.findall(r'(HB.+) = ([\s\S]+?)\$\(NULL\)', makefile):
    files_list = ['src/' + x for x in filter(
        lambda x: x not in BLACKLIST,
        re.sub(r'[\\\t\n]', '', m[1]).strip().split(' ')
    )]

    cmakefile = re.sub(r'\{\{' + m[0] + r'\}\}', '\n  '.join(files_list), cmakefile)

open('CMakeLists.txt', 'w').write(cmakefile)
