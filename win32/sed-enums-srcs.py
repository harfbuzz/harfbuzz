#!/usr/bin/python
#
# Utility script to replace strings in the
# generated enum sources, as needed by the build

# Author: Fan, Chun-wei
# Date: Oct. 5, 2017

import os
import sys
import argparse

from replace import replace_multi

def main(argv):
    parser = argparse.ArgumentParser(description='Replace strings in generated enum sources')
    parser.add_argument('--input', help='input generated temporary enum source',
                        required=True)
    parser.add_argument('--output',
                        help='output generated final enum source', required=True)
    args = parser.parse_args()

    # check whether the generated temporary enum source exists
    if not os.path.exists(args.input):
        raise SystemExit('Specified generated temporary enum source \'%s\' is invalid' % args.input)

    replace_items = {'_t_get_type': '_get_type',
                     '_T (': ' ('}

    # Generate the final enum source
    replace_multi(args.input,
                  args.output,
                  replace_items)

if __name__ == '__main__':
    sys.exit(main(sys.argv))