#!/usr/bin/python

import sys, os, re, difflib, unicodedata, errno, cgi
from itertools import *

diff_symbols = "-+=*&^%$#@!~/"
diff_colors = ['red', 'green', 'blue']

class ColorFormatter:

	class Null:
		@staticmethod
		def start_color (c): return ''
		@staticmethod
		def end_color (): return ''
		@staticmethod
		def escape (s): return s
		@staticmethod
		def newline (): return '\n'

	class ANSI:
		@staticmethod
		def start_color (c):
			return {
				'red': '\033[41;37;1m',
				'green': '\033[42;37;1m',
				'blue': '\033[44;37;1m',
			}[c]
		@staticmethod
		def end_color ():
			return '\033[m'
		@staticmethod
		def escape (s): return s
		@staticmethod
		def newline (): return '\n'

	class HTML:
		@staticmethod
		def start_color (c):
			return '<span style="background:%s">' % c
		@staticmethod
		def end_color ():
			return '</span>'
		@staticmethod
		def escape (s): return cgi.escape (s)
		@staticmethod
		def newline (): return '<br/>\n'

	@staticmethod
	def Auto (argv = [], out = sys.stdout):
		format = ColorFormatter.ANSI
		if "--format" in argv:
			argv.remove ("--format")
			format = ColorFormatter.ANSI
		if "--format=ansi" in argv:
			argv.remove ("--format=ansi")
			format = ColorFormatter.ANSI
		if "--format=html" in argv:
			argv.remove ("--format=html")
			format = ColorFormatter.HTML
		if "--no-format" in argv:
			argv.remove ("--no-format")
			format = ColorFormatter.Null
		return format


class DiffColorizer:

	diff_regex = re.compile ('([a-za-z0-9_]*)([^a-za-z0-9_]?)')

	def __init__ (self, formatter, colors=diff_colors, symbols=diff_symbols):
		self.formatter = formatter
		self.colors = colors
		self.symbols = symbols

	def colorize_lines (self, lines):
		lines = (l if l else '' for l in lines)
		ss = [self.diff_regex.sub (r'\1\n\2\n', l).splitlines (True) for l in lines]
		oo = ["",""]
		st = [False, False]
		for l in difflib.Differ().compare (*ss):
			if l[0] == '?':
				continue
			if l[0] == ' ':
				for i in range(2):
					if st[i]:
						oo[i] += self.formatter.end_color ()
						st[i] = False
				oo = [o + self.formatter.escape (l[2:]) for o in oo]
				continue
			if l[0] in self.symbols:
				i = self.symbols.index (l[0])
				if not st[i]:
					oo[i] += self.formatter.start_color (self.colors[i])
					st[i] = True
				oo[i] += self.formatter.escape (l[2:])
				continue
		for i in range(2):
			if st[i]:
				oo[i] += self.formatter.end_color ()
				st[i] = False
		oo = [o.replace ('\n', '') for o in oo]
		return [s1+s2+self.formatter.newline () for (s1,s2) in zip (self.symbols, oo) if s2]

	def colorize_diff (self, f):
		lines = [None, None]
		for l in f:
			if l[0] not in self.symbols:
				yield self.formatter.escape (l).replace ('\n', self.formatter.newline ())
				continue
			i = self.symbols.index (l[0])
			if lines[i]:
				# Flush
				for line in self.colorize_lines (lines):
					yield line
				lines = [None, None]
			lines[i] = l[1:]
			if (all (lines)):
				# Flush
				for line in self.colorize_lines (lines):
					yield line
				lines = [None, None]
		if (any (lines)):
			# Flush
			for line in self.colorize_lines (lines):
				yield line


class ZipDiffer:

	@staticmethod
	def diff_files (files, symbols=diff_symbols):
		files = tuple (files) # in case it's a generator, copy it
		try:
			for lines in izip_longest (*files):
				if all (lines[0] == line for line in lines[1:]):
					sys.stdout.writelines ([" ", lines[0]])
					continue

				for i, l in enumerate (lines):
					if l:
						sys.stdout.writelines ([symbols[i], l])
		except IOError as e:
			if e.errno != errno.EPIPE:
				print >> sys.stderr, "%s: %s: %s" % (sys.argv[0], e.filename, e.strerror)
				sys.exit (1)


class DiffFilters:

	@staticmethod
	def filter_failures (f):
		for lines in DiffHelpers.separate_test_cases (f):
			if not DiffHelpers.test_passed (lines):
				for l in lines: yield l

class DiffSinks:

	@staticmethod
	def print_stat (f):
		passed = 0
		failed = 0
		for lines in DiffHelpers.separate_test_cases (f):
			if DiffHelpers.test_passed (lines):
				passed += 1
			else:
				failed += 1
		total = passed + failed
		print "%d out of %d tests passed.  %d failed (%g%%)" % (passed, total, failed, 100. * failed / total)

class DiffHelpers:

	@staticmethod
	def separate_test_cases (f):
		'''Reads lines from f, and if the lines have identifiers, ie.
		   have a colon character, groups them by identifier,
		   yielding lists of all lines with the same identifier.'''

		acc = []
		iden = None
		for l in f:
			if ':' not in l:
				if acc: yield acc
				acc = []
				iden = None
				yield [l]
				continue
			l_iden = l[1:l.index (':')]
			if acc and iden != l_iden:
				yield acc
				acc = []
			iden = l_iden
			acc.append (l)
		if acc: yield acc

	@staticmethod
	def test_passed (lines):
		return all (l[0] == ' ' for l in lines)


class FilterHelpers:

	@staticmethod
	def filter_printer_function (filter_callback):
		def printer (f):
			for line in filter_callback (f):
				print line
		return printer

	@staticmethod
	def filter_printer_function_no_newline (filter_callback):
		def printer (f):
			for line in filter_callback (f):
				sys.stdout.writelines ([line])
		return printer


class UtilMains:

	@staticmethod
	def process_multiple_files (callback, mnemonic = "FILE"):

		if "--help" in sys.argv:
			print "Usage: %s %s..." % (sys.argv[0], mnemonic)
			sys.exit (1)

		try:
			files = sys.argv[1:] if len (sys.argv) > 1 else ['-']
			for s in files:
				callback (FileHelpers.open_file_or_stdin (s))
		except IOError as e:
			if e.errno != errno.EPIPE:
				print >> sys.stderr, "%s: %s: %s" % (sys.argv[0], e.filename, e.strerror)
				sys.exit (1)

	@staticmethod
	def process_multiple_args (callback, mnemonic):

		if len (sys.argv) == 1:
			print "Usage: %s %s..." % (sys.argv[0], mnemonic)
			sys.exit (1)

		try:
			for s in sys.argv[1:]:
				callback (s)
		except IOError as e:
			if e.errno != errno.EPIPE:
				print >> sys.stderr, "%s: %s: %s" % (sys.argv[0], e.filename, e.strerror)
				sys.exit (1)

	@staticmethod
	def filter_multiple_strings_or_stdin (callback, mnemonic, \
					      separator = " ", \
					      concat_separator = False):

		if len (sys.argv) == 1 or ('--stdin' in sys.argv and len (sys.argv) != 2):
			print "Usage:\n  %s %s...\nor:\n  %s --stdin" \
			      % (sys.argv[0], mnemonic, sys.argv[0])
			sys.exit (1)

		try:
			if '--stdin' in sys.argv:
				sys.argv.remove ('--stdin')
				while (1):
					line = sys.stdin.readline ()
					if not len (line):
						break
					if line[-1] == '\n':
						line = line[:-1]
					print callback (line)
			else:
				args = sys.argv[1:]
				if concat_separator != False:
					args = [concat_separator.join (args)]
				print separator.join (callback (x) for x in (args))
		except IOError as e:
			if e.errno != errno.EPIPE:
				print >> sys.stderr, "%s: %s: %s" % (sys.argv[0], e.filename, e.strerror)
				sys.exit (1)


class Unicode:

	@staticmethod
	def decode (s):
		return '<' + u','.join ("U+%04X" % ord (u) for u in unicode (s, 'utf-8')).encode ('utf-8') + '>'

	@staticmethod
	def encode (s):
		s = re.sub (r"[<+>,\\uU\n	]", " ", s)
		s = re.sub (r"0[xX]", " ", s)
		return u''.join (unichr (int (x, 16)) for x in s.split (' ') if len (x)).encode ('utf-8')

	shorthands = {
		"ZERO WIDTH NON-JOINER": "ZWNJ",
		"ZERO WIDTH JOINER": "ZWJ",
		"NARROW NO-BREAK SPACE": "NNBSP",
		"COMBINING GRAPHEME JOINER": "CGJ",
		"LEFT-TO-RIGHT MARK": "LRM",
		"RIGHT-TO-LEFT MARK": "RLM",
		"LEFT-TO-RIGHT EMBEDDING": "LRE",
		"RIGHT-TO-LEFT EMBEDDING": "RLE",
		"POP DIRECTIONAL FORMATTING": "PDF",
		"LEFT-TO-RIGHT OVERRIDE": "LRO",
		"RIGHT-TO-LEFT OVERRIDE": "RLO",
	}

	@staticmethod
	def pretty_name (u):
		try:
			s = unicodedata.name (u)
		except ValueError:
			return "XXX"
		s = re.sub (".* LETTER ", "", s)
		s = re.sub (".* VOWEL SIGN (.*)", r"\1-MATRA", s)
		s = re.sub (".* SIGN ", "", s)
		s = re.sub (".* COMBINING ", "", s)
		if re.match (".* VIRAMA", s):
			s = "HALANT"
		if s in Unicode.shorthands:
			s = Unicode.shorthands[s]
		return s

	@staticmethod
	def pretty_names (s):
		s = re.sub (r"[<+>\\uU]", " ", s)
		s = re.sub (r"0[xX]", " ", s)
		s = [unichr (int (x, 16)) for x in re.split ('[, \n]', s) if len (x)]
		return u' + '.join (Unicode.pretty_name (x) for x in s).encode ('utf-8')


class FileHelpers:

	@staticmethod
	def open_file_or_stdin (f):
		if f == '-':
			return sys.stdin
		return file (f)


class Manifest:

	@staticmethod
	def read (s, strict = True):

		if not os.path.exists (s):
			if strict:
				print >> sys.stderr, "%s: %s does not exist" % (sys.argv[0], s)
				sys.exit (1)
			return

		s = os.path.normpath (s)

		if os.path.isdir (s):

			try:
				m = file (os.path.join (s, "MANIFEST"))
				items = [x.strip () for x in m.readlines ()]
				for f in items:
					for p in Manifest.read (os.path.join (s, f)):
						yield p
			except IOError:
				if strict:
					print >> sys.stderr, "%s: %s does not exist" % (sys.argv[0], os.path.join (s, "MANIFEST"))
					sys.exit (1)
				return
		else:
			yield s

	@staticmethod
	def update_recursive (s):

		for dirpath, dirnames, filenames in os.walk (s, followlinks=True):

			for f in ["MANIFEST", "README", "LICENSE", "COPYING", "AUTHORS", "SOURCES", "ChangeLog"]:
				if f in dirnames:
					dirnames.remove (f)
				if f in filenames:
					filenames.remove (f)
			dirnames.sort ()
			filenames.sort ()
			ms = os.path.join (dirpath, "MANIFEST")
			print "  GEN    %s" % ms
			m = open (ms, "w")
			for f in filenames:
				print >> m, f
			for f in dirnames:
				print >> m, f
			for f in dirnames:
				Manifest.update_recursive (os.path.join (dirpath, f))

if __name__ == '__main__':
	pass
