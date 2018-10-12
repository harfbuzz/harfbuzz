#!/usr/bin/python

"""Generator of the function to prohibit certain vowel sequences.

It creates ``preprocess_text_vowel_constraints``, which inserts dotted
circles into sequences prohibited by the USE script development spec.
This function should be used as the ``preprocess_text`` of an
``hb_ot_complex_shaper_t``.

It also creates the helper function ``_output_with_dotted_circle``.
"""

from __future__ import absolute_import, division, print_function, unicode_literals

import collections
try:
	from HTMLParser import HTMLParser
	def write (s):
		print (s.encode ('utf-8'), end='')
except ImportError:
	from html.parser import HTMLParser
	def write (s):
		sys.stdout.flush ()
		sys.stdout.buffer.write (s.encode ('utf-8'))
import itertools
import io
import sys

if len (sys.argv) != 3:
	print ('usage: ./gen-vowel-constraints.py use Scripts.txt', file=sys.stderr)
	sys.exit (1)

try:
	from html import unescape
	def html_unescape (parser, entity):
		return unescape (entity)
except ImportError:
	def html_unescape (parser, entity):
		return parser.unescape (entity)

def expect (condition, message=None):
	if not condition:
		if message is None:
			raise AssertionError
		raise AssertionError (message)

with io.open (sys.argv[2], encoding='utf-8') as f:
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

	def _indent (self, depth):
		return ('  ' * depth).replace ('        ', '\t')

	def __str__ (self, index=0, depth=4):
		s = []
		indent = self._indent (depth)
		if isinstance (self._c, list):
			if len (self._c) == 0:
				s.append ('{}matched = true;\n'.format (indent))
			elif len (self._c) == 1:
				s.append ('{}matched = 0x{:04X}u == buffer->cur ({}).codepoint;\n'.format (indent, next (iter (self._c)), index or ''))
			else:
				s.append ('{}if (0x{:04X}u == buffer->cur ({}).codepoint &&\n'.format (indent, self._c[0], index))
				s.append ('{}buffer->idx + {} < count &&\n'.format (self._indent (depth + 2), len (self._c)))
				for i, cp in enumerate (self._c[1:], start=1):
					s.append ('{}0x{:04X}u == buffer->cur ({}).codepoint{}\n'.format (
						self._indent (depth + 2), cp, index + i, ')' if i == len (self._c) - 1 else ' &&'))
				s.append ('{}{{\n'.format (indent))
				for i in range (len (self._c)):
					s.append ('{}buffer->next_glyph ();\n'.format (self._indent (depth + 1)))
				s.append ('{}buffer->output_glyph (0x25CCu);\n'.format (self._indent (depth + 1)))
				s.append ('{}}}\n'.format (indent))
		else:
			s.append ('{}switch (buffer->cur ({}).codepoint)\n'.format(indent, index or ''))
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

class USESpecParser (HTMLParser):
	"""A parser for the USE script development spec.

	Attributes:
		header (str): The ``updated_at`` timestamp of the spec.
		constraints (Mapping[str, ConstraintSet]): A map of script names
			to the scripts' prohibited sequences.
	"""
	def __init__ (self):
		HTMLParser.__init__ (self)
		self.header = ''
		self.constraints = {}
		# Whether the next <code> contains the vowel constraints.
		self._primed = False
		# Whether the parser is in the <code> element with the constraints.
		self._in_constraints = False
		# The text of the constraints.
		self._constraints = ''

	def handle_starttag (self, tag, attrs):
		if tag == 'meta':
			for attr, value in attrs:
				if attr == 'name' and value == 'updated_at':
					self.header = self.get_starttag_text ()
					break
		elif tag == 'a':
			for attr, value in attrs:
				if attr == 'id' and value == 'ivdvconstraints':
					self._primed = True
					break
		elif self._primed and tag == 'code':
			self._primed = False
			self._in_constraints = True

	def handle_endtag (self, tag):
		self._in_constraints = False

	def handle_data (self, data):
		if self._in_constraints:
			self._constraints += data

	def handle_charref (self, name):
		self.handle_data (html_unescape (self, '&#%s;' % name))

	def handle_entityref (self, name):
		self.handle_data (html_unescape (self, '&%s;' % name))

	def parse (self, filename):
		"""Parse the USE script development spec.

		Args:
			filename (str): The file name of the spec.
		"""
		with io.open (filename, encoding='utf-8') as f:
			self.feed (f.read ())
		expect (self.header, 'No header found')
		for line in self._constraints.splitlines ():
			constraint = [int (cp, 16) for cp in line.split (';')[0].strip ().split (' ')]
			expect (2 <= len (constraint), 'Prohibited sequence is too short: {}'.format (constraint))
			script = scripts[constraint[0]]
			if script in self.constraints:
				self.constraints[script].add (constraint)
			else:
				self.constraints[script] = ConstraintSet (constraint)
		expect (self.constraints, 'No constraints found')

use_parser = USESpecParser ()
use_parser.parse (sys.argv[1])

print ('/* == Start of generated functions == */')
print ('/*')
print (' * The following functions are generated by running:')
print (' *')
print (' *   %s use Scripts.txt' % sys.argv[0])
print (' *')
print (' * on files with these headers:')
print (' *')
print (' * %s' % use_parser.header.strip ())
for line in scripts_header:
	print (' * %s' % line.strip ())
print (' */')
print ()
print ('#ifndef HB_OT_SHAPE_COMPLEX_VOWEL_CONSTRAINTS_HH')
print ('#define HB_OT_SHAPE_COMPLEX_VOWEL_CONSTRAINTS_HH')
print ()

print ('static void')
print ('_output_with_dotted_circle (hb_buffer_t *buffer)')
print ('{')
print ('  hb_glyph_info_t &dottedcircle = buffer->output_glyph (0x25CCu);')
print ('  _hb_glyph_info_reset_continuation (&dottedcircle);')
print ()
print ('  buffer->next_glyph ();')
print ('}')
print ()

print ('static void')
print ('preprocess_text_vowel_constraints (const hb_ot_shape_plan_t *plan,')
print ('\t\t\t\t   hb_buffer_t              *buffer,')
print ('\t\t\t\t   hb_font_t                *font)')
print ('{')
print ('  /* UGLY UGLY UGLY business of adding dotted-circle in the middle of')
print ('   * vowel-sequences that look like another vowel.  Data for each script')
print ('   * collected from the USE script development spec.')
print ('   *')
print ('   * https://github.com/harfbuzz/harfbuzz/issues/1019')
print ('   */')
print ('  bool processed = false;')
print ('  buffer->clear_output ();')
print ('  unsigned int count = buffer->len;')
print ('  switch ((unsigned) buffer->props.script)')
print ('  {')

for script, constraints in sorted (use_parser.constraints.items (), key=lambda s_c: script_order[s_c[0]]):
	print ('    case HB_SCRIPT_{}:'.format (script.upper ()))
	print ('      for (buffer->idx = 0; buffer->idx + 1 < count && buffer->successful;)')
	print ('      {')
	print ('\tbool matched = false;')
	write (str (constraints))
	print ('\tbuffer->next_glyph ();')
	print ('\tif (matched) _output_with_dotted_circle (buffer);')
	print ('      }')
	print ('      processed = true;')
	print ('      break;')
	print ()

print ('    default:')
print ('      break;')
print ('  }')
print ('  if (processed)')
print ('  {')
print ('    if (buffer->idx < count)')
print ('     buffer->next_glyph ();')
print ('    if (likely (buffer->successful))')
print ('      buffer->swap_buffers ();')
print ('  }')
print ('}')

print ()
print ('#endif /* HB_OT_SHAPE_COMPLEX_VOWEL_CONSTRAINTS_HH */')
print ()
print ('/* == End of generated functions == */')
