#!/usr/bin/env python3

import argparse
import enum
import graphite2

from argparse import ArgumentParser


class Dir(enum.IntEnum):
  ltr = 0
  rtl = 1

def usv(v): return int(v,16)
 
 
p = ArgumentParser()
p.add_argument('fontname', help='font file')
p.add_argument('codes', nargs='+', type=usv, help='USV codes')
p.add_argument('--ppm', type=int, default=96, help='Pixels per EM')
p.add_argument('--script', type=bytes, default=b'latn', help='Script tag')
p.add_argument('--dir', type=Dir, choices=[Dir.ltr, Dir.rtl], 
               default=Dir.ltr, 
               help='Text direction')

args = p.parse_args()

face = graphite2.Face(args.fontname)
font = graphite2.Font(face, args.ppm)

args.codes = ''.join(chr(c) for c in args.codes)
seg = graphite2.Segment(font, face, args.script, args.codes, args.dir)
print(f'Glyphs: {[s.gid for s in seg.slots]!r}')

def reordering_clusters(seg):
  slots_first = seg.cinfo(0).before
  slots_last = seg.cinfo(0).after
  char_first = 0
  
  for n in range(1,seg.num_cinfo):
    ci = seg.cinfo(n)
    
    slots_first = min(ci.before, slots_first)
    if ci.before > slots_last:
      yield (char_first, slots_last)
      char_first = n
      slots_first = ci.before
    
    slots_last = max(ci.after, slots_last)
  
  yield (char_first, slots_last)


print(list(reordering_clusters(seg)))


def clusters(seg):
  slots = seg.slots
  bounds = reordering_clusters(seg)
  
  glyphs = [slots[0].index]
  fc,ls = next(bounds)
  
  for s in slots[1:]:
    if s.index > ls:
      fc_,ls = next(bounds)
      if s.insert_before:
        yield (fc_ - fc, glyphs)
        fc = fc_
        glyphs = []
    glyphs.append(s.index)
  yield (seg.num_cinfo - fc, glyphs)

print(list(clusters(seg)))

# 1a23 1a6e 1a55 1a60 1a75 1a26 1a23 1a6e 1a55 1a60 1a75 1a26
# [73, 4, 27, 76, 102, 73), 4, 27, 76, 102, 73, 4, 27, 76, 102]


# 1a23 1a6e 1a55 1a60 1a75 1a26: [73, 4, 27, 76, 102]
# 1a23 1a6e 1a55: [73]




 
