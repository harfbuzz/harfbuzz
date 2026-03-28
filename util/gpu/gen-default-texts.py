#!/usr/bin/env python3

"""Generate line-wrapped default text headers from paragraph files."""

import sys, os

SRCDIR = os.path.dirname(os.path.abspath(__file__))
PERFDIR = os.path.join(SRCDIR, '..', '..', 'perf', 'texts')

texts = [
    ('en-paragraph.txt', 'default_text_en', 150),
    ('fa-paragraph.txt', 'default_text_fa', 165),
]

def wrap_at_width(text, width):
    """Word-wrap a paragraph at approximately `width` characters."""
    lines = []
    for para in text.split('\n'):
        words = para.split(' ')
        line = ''
        for word in words:
            if line and len(line) + 1 + len(word) > width:
                lines.append(line)
                line = word
            else:
                line = line + ' ' + word if line else word
        if line:
            lines.append(line)
    return '\n'.join(lines)

def stringize(varname, text):
    """Convert text to a C string literal."""
    lines = text.split('\n')
    parts = ['static const char *{} =\n'.format(varname)]
    for line in lines:
        line = line.replace('\\', '\\\\').replace('"', '\\"')
        parts.append('"{}\\n"\n'.format(line))
    parts.append(';\n')
    return ''.join(parts)

MAX_TOTAL_LINES = 70

all_wrapped = []
for filename, varname, width in texts:
    path = os.path.join(PERFDIR, filename)
    with open(path, 'r') as f:
        text = f.read().strip()
    wrapped = wrap_at_width(text, width)
    all_wrapped.append((varname, wrapped))
    hh = stringize(varname, wrapped)
    outpath = os.path.join(SRCDIR, varname.replace('default_text_', 'default-text-') + '.hh')
    with open(outpath, 'w') as f:
        f.write(hh)
    print('Generated: {}'.format(outpath))

# Generate combined text cropped to MAX_TOTAL_LINES / num_texts per text.
lines_per = MAX_TOTAL_LINES // len(all_wrapped)
combined_lines = []
for varname, wrapped in all_wrapped:
    lines = wrapped.split('\n')[:lines_per]
    combined_lines.extend(lines)
combined = '\n'.join(combined_lines)
hh = stringize('default_text_combined', combined)
outpath = os.path.join(SRCDIR, 'default-text-combined.hh')
with open(outpath, 'w') as f:
    f.write(hh)
print('Generated: {}'.format(outpath))
