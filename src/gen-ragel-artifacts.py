#!/usr/bin/env python3

import io, os, re, sys, subprocess, shutil

if len (sys.argv) < 3:
	ragel_sources = [x for x in os.listdir ('.') if x.endswith ('.rl')]
else:
	os.chdir(sys.argv[1])
	ragel_sources = sys.argv[2:]

ragel = shutil.which ('ragel')

if not ragel:
	print ('You have to install ragel if you are going to develop HarfBuzz itself')
	exit (1)

if not len (ragel_sources):
	exit (77)

for rl in ragel_sources:
	hh = rl.replace ('.rl', '.hh')
	subprocess.Popen ([ragel, '-e', '-F1', '-o', hh, rl]).wait ()
