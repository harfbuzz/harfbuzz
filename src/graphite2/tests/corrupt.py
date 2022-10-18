#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright 2011, SIL International, All rights reserved.

import argparse
import functools
import os
import pathlib
import shutil


def revert(args):
    bkup_path = args.file.with_suffix(args.file.suffix + args.backup_suffix)
    if bkup_path.exists():
        bkup_path.replace(args.file)


def corrupt(args):
    revert(args)
    if args.backup:
        bkup_path = args.file.with_suffix(args.file.suffix
                                          + args.backup_suffix)
        shutil.copy2(args.file, bkup_path)
    with args.file.open('r+b', buffering=0) as f:
        f.seek(args.offset)
        f.write(args.value.to_bytes((args.value.bit_length() + 7)//8,
                                    byteorder='big'))


int_parser = functools.partial(int, base=0)
parser = argparse.ArgumentParser(
        description="Write a value into a file at an offset, making a backup.")
parser.add_argument("file", type=pathlib.Path,
                    help="File to corrupt")
parser.add_argument("--backup-suffix", default=os.extsep + "pristine",
                    metavar="SUFFIX",
                    help="suffix for uncorrupted copy of the file "
                         "[default: %(default)s]")
cmd_parser = parser.add_subparsers(dest='mode')
cmd_parser.add_parser(
            'revert', aliases=['r', 're', 'rev'],
            help='Revert the damage, restore backup').set_defaults(mode=revert)

damage = cmd_parser.add_parser(
            'damage', aliases=['d', 'da', 'dam'],
            help='Corrupt the file at the offset with the value')
damage.set_defaults(mode=corrupt)
damage.add_argument("offset", type=int_parser,
                    help="Offset into file in bytes")
damage.add_argument("value", type=int_parser,
                    help="Value to overwrite at OFFSET")
damage.add_argument("-b", "--backup",
                    action="store_true", default=True,
                    help="create a backup of the uncorrupted original"
                         " [default: %(default)s]")
damage.add_argument("--no-backup", dest='backup',
                    action="store_false",
                    help="do not create a backup of the uncorrupted original.")

args = parser.parse_args()
args.mode(args)
