#!/usr/bin/env python3
# Copyright 2014  Emmanuele Bassi
#
# SPDX-License-Identifier: MIT
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

import sys
import re

NUMBER_REGEX = re.compile(r'([0-9])([a-z])')

def to_camel_case(text):
    # We only care about Harfbuzz types
    if not text.startswith('hb_') and not text.endswith('_t'):
        return text

    res = []
    for token in text[:-2].split('_'):
        uc_token = token.title()

        matches = NUMBER_REGEX.match(uc_token)
        if matches and matches.group(2):
            uc_token = ''.join([matches.group(1), matches.group(2).title])

        res.append(uc_token)

    return ''.join(res)

if __name__ == '__main__':
    in_text = sys.stdin.read()
    sys.stdout.write(to_camel_case(in_text))
