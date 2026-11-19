#!/usr/bin/env python3

"""Generator of the function to prohibit certain vowel sequences.

It creates ``_hb_preprocess_text_vowel_constraints``, which inserts dotted
circles into sequences prohibited by the USE script development spec.
This function should be used as the ``preprocess_text`` of an
``hb_ot_shaper_t``.

usage: ./gen-vowel-constraints.py [--rust] ms-use/IndicShapingInvalidCluster.txt Scripts.txt

Input file:
* https://unicode.org/Public/UCD/latest/ucd/Scripts.txt
"""

import collections
import os
def write (s):
	sys.stdout.flush ()
	sys.stdout.buffer.write (s.encode ('utf-8'))
import sys

if len (sys.argv) > 1 and sys.argv[1] == '--rust':
	del sys.argv[1]
	output_rust = True
else:
	output_rust = False

if len (sys.argv) != 3:
	sys.exit (__doc__)

with open (sys.argv[2], encoding='utf-8') as f:
	scripts_header = [f.readline () for i in range (2)]
	scripts = {}
	script_order = {}
	for line in f:
		j = line.find ('#')
		if j >= 0:
			line = line[:j]
		fields = [x.strip () for x in line.split (';')]
		if len (fields) == 1:
			continue
		uu = fields[0].split ('..')
		start = int (uu[0], 16)
		if len (uu) == 1:
			end = start
		else:
			end = int (uu[1], 16)
		script = fields[1]
		for u in range (start, end + 1):
			scripts[u] = script
		if script not in script_order:
			script_order[script] = start

class ConstraintSet (object):
	"""A set of prohibited code point sequences.

	Args:
		constraint (List[int]): A prohibited code point sequence.

	"""
	def __init__ (self, constraint):
		# Either a list or a dictionary. As a list of code points, it
		# represents a prohibited code point sequence. As a dictionary,
		# it represents a set of prohibited sequences, where each item
		# represents the set of prohibited sequences starting with the
		# key (a code point) concatenated with any of the values
		# (ConstraintSets).
		self._c = constraint

	def add (self, constraint):
		"""Add a constraint to this set."""
		if not constraint:
			return
		first = constraint[0]
		rest = constraint[1:]
		if isinstance (self._c, list):
			if constraint == self._c[:len (constraint)]:
				self._c = constraint
			elif self._c != constraint[:len (self._c)]:
				self._c = {self._c[0]: ConstraintSet (self._c[1:])}
		if isinstance (self._c, dict):
			if first in self._c:
				self._c[first].add (rest)
			else:
				self._c[first] = ConstraintSet (rest)

	@staticmethod
	def _indent (depth):
		return ('  ' * depth).replace ('        ', '\t')

	def __str__ (self, index=0, depth=4):
		s = []
		indent = self._indent (depth)
		if isinstance (self._c, list):
			if len (self._c) == 0:
				assert index == 2, 'Cannot use `matched` for this constraint; the general case has not been implemented'
				s.append ('{}matched = true;\n'.format (indent))
			elif len (self._c) == 1:
				assert index == 1, 'Cannot use `matched` for this constraint; the general case has not been implemented'
				s.append ('{}matched = 0x{:04X}u == _vowel_constraint_glyph (buffer, {});\n'.format (indent, next (iter (self._c)), index))
			else:
				s.append ('{}if (0x{:04X}u == _vowel_constraint_glyph (buffer, {}) &&\n'.format (indent, self._c[0], index))
				if index:
					s.append ('{}buffer->idx + {} < count &&\n'.format (self._indent (depth + 2), index + 1))
				for i, cp in enumerate (self._c[1:], start=1):
					s.append ('{}0x{:04X}u == _vowel_constraint_glyph (buffer, {}){}\n'.format (
						self._indent (depth + 2), cp, index + i, ')' if i == len (self._c) - 1 else ' &&'))
				s.append ('{}{{\n'.format (indent))
				for i in range (index):
					s.append ('{}_next_vowel_constraint_glyph (buffer);\n'.format (self._indent (depth + 1)))
				s.append ('{}matched = true;\n'.format (self._indent (depth + 1)))
				s.append ('{}}}\n'.format (indent))
		else:
			s.append ('{}switch (_vowel_constraint_glyph (buffer, {}))\n'.format(indent, index))
			s.append ('{}{{\n'.format (indent))
			cases = collections.defaultdict (set)
			for first, rest in sorted (self._c.items ()):
				cases[rest.__str__ (index + 1, depth + 2)].add (first)
			for body, labels in sorted (cases.items (), key=lambda b_ls: sorted (b_ls[1])[0]):
				for i, cp in enumerate (sorted (labels)):
					if i % 4 == 0:
						s.append (self._indent (depth + 1))
					else:
						s.append (' ')
					s.append ('case 0x{:04X}u:{}'.format (cp, '\n' if i % 4 == 3 else ''))
				if len (labels) % 4 != 0:
					s.append ('\n')
				s.append (body)
				s.append ('{}break;\n'.format (self._indent (depth + 2)))
			s.append ('{}}}\n'.format (indent))
		return ''.join (s)

	def rust_str (self, index=0):
		s = []
		if isinstance (self._c, list):
			if len (self._c) == 0:
				assert index == 2, 'Cannot use `matched` for this constraint; the general case has not been implemented'
				s.append ('matched = true;\n')
			elif len (self._c) == 1:
				assert index == 1, 'Cannot use `matched` for this constraint; the general case has not been implemented'
				s.append ('matched = 0x{:04X} == cur_glyph(buffer, {});\n'.format (next (iter (self._c)), index))
			else:
				s.append ('if 0x{:04X} == cur_glyph(buffer, {}) &&\n'.format (self._c[0], index))
				if index:
					s.append ('buffer.idx + {} < buffer.len &&\n'.format (index + 1))
				for i, cp in enumerate (self._c[1:], start=1):
					s.append ('0x{:04X} == cur_glyph(buffer, {}){}\n'.format (
						cp, index + i, '' if i == len (self._c) - 1 else ' &&'))
				s.append ('{\n')
				for i in range (index + 1):
					s.append ('next_vowel_constraint_glyph(buffer);\n')
				s.append ('output_dotted_circle(buffer);\n')
				s.append ('}\n')
		else:
			s.append ('match cur_glyph(buffer, {}) {{\n'.format (index))
			cases = collections.defaultdict (set)
			for first, rest in sorted (self._c.items ()):
				cases[rest.rust_str (index + 1)].add (first)
			items = sorted (cases.items (), key=lambda b_ls: sorted (b_ls[1])[0])
			for ii, (body, labels) in enumerate (items):
				for i, cp in enumerate (sorted (labels)):
					if i == len (labels) - 1:
						s.append (' 0x{:04X} => {{ {}'.format (cp, '\n' if i % 4 == 3 else ''))
					else:
						s.append (' 0x{:04X} | {}'.format (cp, '\n' if i % 4 == 3 else ''))
				s.append (body)
				s.append ('}')
				if ii == len (items) - 1:
					s.append ('_ => {}')
			s.append ('}\n')
		return ''.join (s)

constraints = {}
with open (sys.argv[1], encoding='utf-8') as f:
	constraints_header = []
	while True:
		line = f.readline ().strip ()
		if line == '#':
			break
		constraints_header.append(line)
	for line in f:
		j = line.find ('#')
		if j >= 0:
			line = line[:j]
		constraint = [int (cp, 16) for cp in line.split (';')[0].split ()]
		if not constraint: continue
		assert 2 <= len (constraint), 'Prohibited sequence is too short: {}'.format (constraint)
		script = scripts[constraint[0]]
		if script in constraints:
			constraints[script].add (constraint)
		else:
			constraints[script] = ConstraintSet (constraint)
		assert constraints, 'No constraints found'

print ('/* == Start of generated functions == */')
if output_rust:
	print ('// WARNING: this file was generated by ../../harfbuzz/src/gen-vowel-constraints.py --rust')
	print ()
	print ('#![allow(clippy::single_match)]')
	print ()
	print ('use crate::buffer::Buffer;')
	print ('use crate::BufferFlags;')
	print ('use crate::Script;')
	print ()
	print ('/// Look ahead across default ignorables; 0 matches nothing at end of input.')
	print ('fn cur_glyph(buffer: &Buffer, i: usize) -> u32 {')
	print ('    let mut pos = buffer.idx;')
	print ('    for _ in 0..i {')
	print ('        if pos >= buffer.len { return 0; }')
	print ('        pos += 1;')
	print ('        while pos < buffer.len && buffer.info.get(pos).is_some_and(|info| info.is_default_ignorable()) {')
	print ('            pos += 1;')
	print ('        }')
	print ('    }')
	print ('    if pos < buffer.len { buffer.info.get(pos).map_or(0, |info| info.glyph_id) } else { 0 }')
	print ('}')
	print ()
	print ('fn next_vowel_constraint_glyph(buffer: &mut Buffer) {')
	print ('    if !buffer.successful { return; }')
	print ('    buffer.next_glyph();')
	print ('    while buffer.successful && buffer.cur(0).is_some_and(|info| info.is_default_ignorable()) {')
	print ('        buffer.next_glyph();')
	print ('    }')
	print ('}')
	print ()
	print ('fn output_dotted_circle(buffer: &mut Buffer) {')
	print ('    buffer.output_glyph(0x25CC);')
	print ('    if !buffer.successful { return; }')
	print ('    let mut scratch_flags = buffer.scratch_flags;')
	print ('    {')
	print ('        let out_idx = buffer.out_len - 1;')
	print ('        if let Some(info) = buffer.out_info_mut().get_mut(out_idx) {')
	print ('            info.init_unicode_props(&mut scratch_flags);')
	print ('            info.clear_continuation();')
	print ('        }')
	print ('    }')
	print ('    buffer.scratch_flags = scratch_flags;')
	print ('}')
	print ()
	print ('fn output_with_dotted_circle(buffer: &mut Buffer) {')
	print ('    output_dotted_circle(buffer);')
	print ('    next_vowel_constraint_glyph(buffer);')
	print ('}')
	print ()
	print ('pub fn preprocess_text_vowel_constraints(buffer: &mut Buffer) {')
	print ('    if buffer.flags.contains(BufferFlags::DO_NOT_INSERT_DOTTED_CIRCLE) {')
	print ('        return;')
	print ('    }')
	print ()
	print ('    // UGLY UGLY UGLY business of adding dotted-circle in the middle of')
	print ('    // vowel-sequences that look like another vowel.  Data for each script')
	print ('    // collected from the USE script development spec.')
	print ('    // Default ignorables are transparent between listed code points.')
	print ('    //')
	print ('    // https://github.com/harfbuzz/harfbuzz/issues/1019')
	print ('    buffer.clear_output();')
	print ('    match buffer.script {')

	for script, constraints in sorted (constraints.items (), key=lambda s_c: script_order[s_c[0]]):
		print ('        Some(Script::{}) => {{'.format (script.upper ()))
		print ('            buffer.idx = 0;')
		print ('            while buffer.idx + 1 < buffer.len {')
		print ('                let start = buffer.out_len;')
		print ('                #[allow(unused_mut)]')
		print ('                let mut matched = false;')
		write (constraints.rust_str ())
		print ('                next_vowel_constraint_glyph(buffer);')
		print ('                if matched {')
		print ('                    output_with_dotted_circle(buffer);')
		print ('                    // The dotted circle depends on the surrounding context, so the')
		print ('                    // matched run (possibly split across clusters by a transparent')
		print ('                    // default ignorable) must not be broken or concatenated.')
		print ('                    buffer.unsafe_to_break_in_outbuffer(start, buffer.out_len);')
		print ('                }')
		print ('            }')
		print ('        }')
		print ()

	print ('        _ => {}')
	print ('    }')
	print ('    buffer.sync();')
	print ('}')
else:
	print ('/*')
	print (' * The following functions are generated by running:')
	print (' *')
	print (' *   %s ms-use/IndicShapingInvalidCluster.txt Scripts.txt' % os.path.basename (sys.argv[0]))
	print (' *')
	print (' * on files with these headers:')
	print (' *')
	for line in constraints_header:
		print (' * %s' % line.strip ())
	print (' *')
	for line in scripts_header:
		print (' * %s' % line.strip ())
	print (' */')
	print ()
	print ('#include "hb.hh"')
	print ()
	print ('#ifndef HB_NO_OT_SHAPE')
	print ()
	print ('#include "hb-ot-shaper-vowel-constraints.hh"')
	print ()
	print ('static hb_codepoint_t')
	print ('_vowel_constraint_glyph (const hb_buffer_t *buffer, unsigned int i)')
	print ('{')
	print ('  unsigned int pos = buffer->idx;')
	print ('  for (unsigned int j = 0; j < i && pos < buffer->len; j++)')
	print ('  {')
	print ('    pos++;')
	print ('    while (pos < buffer->len && _hb_glyph_info_is_default_ignorable (&buffer->info[pos]))')
	print ('      pos++;')
	print ('  }')
	print ('  return pos < buffer->len ? buffer->info[pos].codepoint : 0;')
	print ('}')
	print ()
	print ('static void')
	print ('_next_vowel_constraint_glyph (hb_buffer_t *buffer)')
	print ('{')
	print ('  if (unlikely (!buffer->successful)) return;')
	print ('  do')
	print ('    (void) buffer->next_glyph ();')
	print ('  while (buffer->successful && buffer->idx < buffer->len &&')
	print ('         _hb_glyph_info_is_default_ignorable (&buffer->cur()));')
	print ('}')
	print ()
	print ('static void')
	print ('_output_dotted_circle (hb_buffer_t *buffer)')
	print ('{')
	print ('  if (unlikely (!buffer->output_glyph (0x25CCu)))')
	print ('    return;')
	print ('  _hb_glyph_info_set_unicode_props (&buffer->prev(), buffer);')
	print ('  _hb_glyph_info_clear_continuation (&buffer->prev());')
	print ('}')
	print ()
	print ('static void')
	print ('_output_with_dotted_circle (hb_buffer_t *buffer)')
	print ('{')
	print ('  _output_dotted_circle (buffer);')
	print ('  _next_vowel_constraint_glyph (buffer);')
	print ('}')
	print ()
	print ('void')
	print ('_hb_preprocess_text_vowel_constraints (const hb_ot_shape_plan_t *plan HB_UNUSED,')
	print ('\t\t\t\t       hb_buffer_t              *buffer,')
	print ('\t\t\t\t       hb_font_t                *font HB_UNUSED)')
	print ('{')
	print ('#ifdef HB_NO_OT_SHAPER_VOWEL_CONSTRAINTS')
	print ('  return;')
	print ('#endif')
	print ('  if (buffer->flags & HB_BUFFER_FLAG_DO_NOT_INSERT_DOTTED_CIRCLE)')
	print ('    return;')
	print ()
	print ('  /* UGLY UGLY UGLY business of adding dotted-circle in the middle of')
	print ('   * vowel-sequences that look like another vowel.  Data for each script')
	print ('   * collected from the USE script development spec.')
	print ('   * Default ignorables are transparent between listed code points.')
	print ('   *')
	print ('   * https://github.com/harfbuzz/harfbuzz/issues/1019')
	print ('   */')
	print ('  buffer->clear_output ();')
	print ('  unsigned int count = buffer->len;')
	print ('  switch ((unsigned) buffer->props.script)')
	print ('  {')

	for script, constraints in sorted (constraints.items (), key=lambda s_c: script_order[s_c[0]]):
		print ('    case HB_SCRIPT_{}:'.format (script.upper ()))
		print ('      for (buffer->idx = 0; buffer->idx + 1 < count && buffer->successful;)')
		print ('      {')
		print ('\tunsigned int start = buffer->out_len;')
		print ('\tbool matched = false;')
		write (str (constraints))
		print ('\t_next_vowel_constraint_glyph (buffer);')
		print ('\tif (matched)')
		print ('\t{')
		print ('\t  _output_with_dotted_circle (buffer);')
		print ('\t  /* The dotted circle depends on the surrounding context, so the')
		print ('\t   * matched run (possibly split across clusters by a transparent')
		print ('\t   * default ignorable) must not be broken or concatenated. */')
		print ('\t  buffer->unsafe_to_break_in_outbuffer (start, buffer->out_len);')
		print ('\t}')
		print ('      }')
		print ('      break;')
		print ()

	print ('    default:')
	print ('      break;')
	print ('  }')
	print ('  buffer->sync ();')
	print ('}')
	print ()
	print ()
	print ('#endif')
print ('/* == End of generated functions == */')
