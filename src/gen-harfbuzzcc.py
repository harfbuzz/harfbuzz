#!/usr/bin/env python3

"This tool is intended to be used from meson"

import os, sys, shutil, pathlib

if len (sys.argv) < 3:
	sys.exit (__doc__)

OUTPUT = sys.argv[1]
CURRENT_SOURCE_DIR = sys.argv[2]

include_features_h = False
feature_defines = []
groups = []
current_guard = None
current_sources = []

def flush_group ():
	global current_sources
	if current_sources:
		groups.append ((current_guard, current_sources))
		current_sources = []

def to_relative_include_path (x):
	abs_x = os.path.abspath (x)
	source_candidate = os.path.join (CURRENT_SOURCE_DIR, os.path.basename (abs_x))
	if os.path.isfile (source_candidate):
		abs_x = source_candidate
	return pathlib.Path (os.path.relpath (abs_x, CURRENT_SOURCE_DIR)).as_posix ()

for arg in sys.argv[3:]:
	if arg == "--include-features-h":
		include_features_h = True
	elif arg.startswith ("--feature-define="):
		feature, define = arg[len ("--feature-define="):].split (":", 1)
		feature_defines.append ((feature, define))
	elif arg.startswith ("--guard="):
		flush_group ()
		current_guard = arg[len ("--guard="):]
	elif arg == "--end-guard":
		flush_group ()
		current_guard = None
	else:
		current_sources.append (arg)

flush_group ()

lines = []

if include_features_h:
	lines.append ("#ifndef HB_NO_FEATURES_H\n")
	lines.append ('#include "hb.h"\n')
	lines.append ('#include "hb-features.h"\n')
	lines.append ("#endif\n")
	lines.append ("\n")

for feature, define in feature_defines:
	lines.append ("#ifdef {0}\n".format (feature))
	lines.append ("#ifndef {0}\n".format (define))
	lines.append ("#define {0} 1\n".format (define))
	lines.append ("#endif\n")
	lines.append ("#endif\n")
	lines.append ("\n")

for guard, sources in groups:
	group_lines = []
	for x in sorted (set (sources)):
		if not x.endswith (".cc"):
			continue
		group_lines.append ('#include "{}"\n'.format (to_relative_include_path (x)))
	if not group_lines:
		continue
	if guard:
		lines.append ("#if {0}\n".format (guard))
	lines.extend (group_lines)
	if guard:
		lines.append ("#endif\n")
	lines.append ("\n")

with open (OUTPUT, "wb") as f:
	f.write ("".join (lines).encode ())

# copy it also to the source tree, but only if it has changed
baseline_filename = os.path.join (CURRENT_SOURCE_DIR, os.path.basename (OUTPUT))
if not os.path.exists (baseline_filename):
	shutil.copyfile (OUTPUT, baseline_filename)
else:
	with open(baseline_filename, "rb") as baseline:
		with open(OUTPUT, "rb") as generated:
			if baseline.read() != generated.read():
				shutil.copyfile (OUTPUT, baseline_filename)
