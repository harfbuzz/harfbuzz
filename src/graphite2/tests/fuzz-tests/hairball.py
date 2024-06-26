#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright 2012, SIL International, All rights reserved.
import argparse
import pathlib
import struct
from typing import List, Tuple

ParamBlock = bytes
Names = List[str]


def map_common(args) -> Tuple[ParamBlock, Names]:
    return (struct.pack("<3L2HB",
                        args.scriptid,
                        args.langid,
                        tag4cc(args.feat[0]), args.feat[1],
                        0x409,
                        _encodings[args.encoding]),
            [args.font.stem,
             args.feat[0] and '{0[0]!s}={0[1]:d}'.format(args.feat),
             args.encoding])


def map_segment(args) -> Tuple[ParamBlock, Names]:
    ps, ns = map_common(args)
    text = args.text.read_text(encoding=args.encoding)
    return (ps + struct.pack("<B128s", _bidi[args.direction], text),
            ns + [args.text.stem, args.direction])


_encodings = {
    'utf8':  1,
    'utf16': 2,
    'utf32': 4
}

_bidi = {'ltr': 0, 'rtl': 1}


def feature(arg: str) -> Tuple[str, int]:
    k, v = arg.split('=')
    return (k.strip(), int(v.strip(), 0))


def tag4cc(arg: str) -> int:
    return struct.unpack_from('>L', bytes(arg, 'ascii') + b'\0'*4)[0]


parser = argparse.ArgumentParser(description='Generate fuzzer seed file')
formatparsers = parser.add_subparsers(metavar='fuzzer-target',
                                      dest='format_spec')

common_parser = argparse.ArgumentParser(add_help=False)
common_parser.add_argument('font', type=pathlib.Path,
                           help='Graphite enabled font file.')
common_parser.add_argument('langid', type=tag4cc,
                           help='Language Id tag')
common_parser.add_argument('scriptid', type=tag4cc,
                           help='Script Id tag')
common_parser.add_argument('encoding', choices=_encodings,
                           help='UTF encoding form')
common_parser.add_argument('--feat', type=feature, default=('', 0xffff),
                           help='A feature to set')

font_parser = formatparsers.add_parser('font', help='font API fuzzer',
                                       parents=[common_parser])
font_parser.set_defaults(format_spec=map_common)

seg_parser = formatparsers.add_parser('segment', help='segment API fuzzer',
                                      aliases=['seg'],
                                      parents=[common_parser])
seg_parser.add_argument('text', type=pathlib.Path,
                        help='Text file to fill test string from')
seg_parser.add_argument('direction', nargs='?', choices=_bidi, default='ltr',
                        help='Text direction')
seg_parser.set_defaults(format_spec=map_segment)

args = parser.parse_args()

params, names = args.format_spec(args)
outname = '_'.join(filter(bool, names)) + '.fuzz'

with open(outname, "wb") as f:
    f.write(args.font.read_bytes())
    f.write(params)
