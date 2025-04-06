#!/usr/bin/env python3

import sys
import os

if len(sys.argv) < 2:
    print("Usage: python glob.py [DIR]...")
    sys.exit(1)

for arg in sys.argv[1:]:
    if os.path.isdir(arg):
        for dirpath, dirnames, filenames in os.walk(arg):
            for filename in filenames:
                print(os.path.join(dirpath, filename))
    elif os.path.isfile(arg):
        print(arg)
