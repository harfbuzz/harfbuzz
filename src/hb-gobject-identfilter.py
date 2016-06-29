# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2014 Simon Feltman <sfeltman@gnome.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
# USA

"""
Script which reads identifiers in the form of "foo_bar_t" from stdin and
translates them to title case like "FooBar", stripping the "_t" suffix.
This also adds a special case where a context like identifier with the same
name as the library is translated into "Context" in GI (useful in situations
like cairo_t -> Context.
"""

import sys


def ensure_title_case(text):
    # Strip off "_t" suffix.
    if text.endswith('_t'):
        text = text[:-2]
    if text == 'hb_unicode_funcs': return 'hb_unicodeFuncs'
    if text == 'hb_unicode_combining_class': return 'hb_unicodeCombiningClass'
    if text == 'hb_unicode_general_category': return 'hb_unicodeGeneralCategory'

    # Split text on underscores and re-join title casing each part
    #text = ''.join(part.title() for part in text.split('_'))

    return text


if __name__ == '__main__':
    text = ensure_title_case(sys.stdin.read())
    sys.stdout.write(text)
