#!/usr/bin/python

import sys, os, re, difflib, unicodedata

class Colors:
	class Null:
		red = ''
		green = ''
		end = ''
	class ANSI:
		red = '\033[41;37;1m'
		green = '\033[42;37;1m'
		end = '\033[m'
	class HTML:
		red = '<span style="color:red">'
		green = '<span style="color:green">'
		end = '</span>'

	@staticmethod
	def Auto (argv = [], out = sys.stdout):
		if "--color" in argv or os.isatty (out.fileno ()):
			if "--color" in sys.argv[1:]:
				argv.remove ("--color")
			return Colors.ANSI
		else:
			return Colors.Null

	@staticmethod
	def Default (argv = []):
		return Colors.ANSI


class FancyDiffer:

	diff_regex = re.compile ('([a-za-z0-9_]*)([^a-za-z0-9_]?)')

	@staticmethod
	def diff_lines (l1, l2, colors=Colors.Null):

		ss = [FancyDiffer.diff_regex.sub (r'\1\n\2\n', l).splitlines (True) for l in (l1, l2)]
		oo = ["",""]
		st = [False, False]
		for l in difflib.Differ().compare (*ss):
			if l[0] == '?':
				continue
			if l[0] == ' ':
				for i in range(2):
					if st[i]:
						oo[i] += colors.end
						st[i] = False
				oo = [o + l[2:] for o in oo]
				continue
			if l[0] == '-':
				if not st[0]:
					oo[0] += colors.red
					st[0] = True
				oo[0] += l[2:]
				continue
			if l[0] == '+':
				if not st[1]:
					oo[1] += colors.green
					st[1] = True
				oo[1] += l[2:]
		for i in range(2):
			if st[i]:
				oo[i] += colors.end
				st[i] = 0
		oo = [o.replace ('\n', '') for o in oo]
		if oo[0] == oo[1]:
			return [' ', oo[0], '\n']
		return ['-', oo[0], '\n', '+', oo[1], '\n']

	@staticmethod
	def diff_files (f1, f2, colors=Colors.Null):
		for (l1,l2) in zip (f1, f2):
			if l1 == l2:
				sys.stdout.writelines ([" ", l1])
				continue

			sys.stdout.writelines (FancyDiffer.diff_lines (l1, l2, colors))
		# print out residues
		for l in f1:
			sys.stdout.writelines (["-", colors.red, l1, colors.end])
		for l in f1:
			sys.stdout.writelines (["-", colors.green, l1, colors.end])


class DiffFilters:

	@staticmethod
	def filter_failures (f):
		for l in f:
			if l[0] in '-+':
				sys.stdout.writelines (l)


class ShapeFilters:

	@staticmethod
	def filter_failures (f):
		for l in f:
			if l[0] in '-+':
				sys.stdout.writelines (l)


class UtilMains:

	@staticmethod
	def process_multiple_files (callback):

		if len (sys.argv) == 1:
			print "Usage: %s FILE..." % sys.argv[0]
			sys.exit (1)

		for s in sys.argv[1:]:
			callback (FileHelpers.open_file_or_stdin (s))

	@staticmethod
	def filter_multiple_strings_or_stdin (callback, string_mnemonic, \
					      separator = " ", \
					      concat_separator = False):

		if len (sys.argv) == 1 or ('--stdin' in sys.argv and len (sys.argv) != 2):
			print "Usage:\n  %s %s...\nor:\n  %s --stdin" \
			      % (sys.argv[0], string_mnemonic, sys.argv[0])
			sys.exit (1)

		if '--stdin' in sys.argv:
			sys.argv.remove ('--stdin')
			while (1):
				line = sys.stdin.readline ()
				if not len (line):
					break
				print callback (line)
		else:
			args = sys.argv[1:]
			if concat_separator != False:
				args = [concat_separator.join (args)]
			print separator.join (callback (x) for x in (args))


class Unicode:

	@staticmethod
	def decode (s):
		return '<' + ','.join ("U+%04X" % ord (u) for u in unicode (s, 'utf8')) + '>'

	@staticmethod
	def encode (s):
		s = re.sub (r"[<+>\\uU]", " ", s)
		s = re.sub (r"0[xX]", " ", s)
		return u''.join (unichr (int (x, 16)) for x in re.split ('[, \n]', s) if len (x))

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
		return ' + '.join (Unicode.pretty_name (x) for x in s)


class FileHelprs:

	@staticmethod
	def open_file_or_stdin (f):
		if f == '-':
			return sys.stdin
		return file (f)

if __name__ == '__main__':
	pass
