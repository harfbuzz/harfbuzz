#!/usr/bin/env python3

import os
import re
import shlex
import subprocess
import sys
from difflib import unified_diff

print("TAP version 14")

args = sys.argv[1:]
if not args or "hb-draw" not in args[0] or not os.path.exists(args[0]):
    sys.exit("First argument does not seem to point to usable hb-draw.")
hb_draw, args = args[0], args[1:]

if not args:
    args = ["-"]

env = os.environ.copy()
env["LC_ALL"] = "C"
EXE_WRAPPER = os.environ.get("MESON_EXE_WRAPPER")


def resolve_path(base, path):
    if os.path.isabs(path):
        return path
    return os.path.normpath(os.path.join(base, path))


def run_draw(font_path, options, text, unicodes):
    cmd = [hb_draw]
    option_argv = []
    if options:
        option_argv = shlex.split(options)
        cmd.extend(option_argv)

    # Prefer --unicodes to keep argv ASCII and avoid locale-dependent UTF-8
    # decoding failures when LC_ALL=C.
    has_text_input_option = False
    for opt in option_argv:
        if opt in ("-t", "--text", "-u", "--unicodes"):
            has_text_input_option = True
            break
        if opt.startswith("--text=") or opt.startswith("--unicodes="):
            has_text_input_option = True
            break

    if has_text_input_option:
        cmd.extend([font_path, text])
    else:
        cmd.extend([f"--unicodes={unicodes}", font_path])
    if EXE_WRAPPER:
        cmd = [EXE_WRAPPER] + cmd
    return subprocess.run(cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)


def parse_unicodes(s):
    # Match test/shape/hb_test_tools.py Unicode.parse semantics.
    s = re.sub(r"0[xX]", " ", s)
    s = re.sub(r"[<+\->{},;&#\\xXuUnNiI\n\t]", " ", s)
    try:
        return "".join(chr(int(x, 16)) for x in s.split())
    except ValueError as e:
        raise ValueError(f"invalid unicode codepoint list: {s!r}") from e


number = 0
fails = 0

for filename in args:
    if filename == "-":
        f = sys.stdin
        base = os.getcwd()
        print("# Running tests from standard input")
    else:
        f = open(filename, encoding="utf-8")
        base = os.path.dirname(os.path.abspath(filename))
        print("# Running tests in " + filename)

    for lineno, line in enumerate(f, start=1):
        line = line.strip()
        if not line or line.startswith("#"):
            continue

        number += 1
        fields = line.split(";", 3)
        if len(fields) != 4:
            fails += 1
            print(f"not ok {number} - parse:{filename}:{lineno}")
            print("# malformed test line, expected 4 ';'-separated fields")
            continue

        font_file, options, unicodes, expected_file = fields
        name = expected_file
        font_path = resolve_path(base, font_file)
        expected_path = resolve_path(base, expected_file)

        if not os.path.exists(font_path):
            fails += 1
            print(f"not ok {number} - {name}")
            print(f"# font file not found: {font_path}")
            continue

        if not os.path.exists(expected_path):
            fails += 1
            print(f"not ok {number} - {name}")
            print(f"# expected output file not found: {expected_path}")
            continue

        try:
            text = parse_unicodes(unicodes)
        except ValueError as e:
            fails += 1
            print(f"not ok {number} - {name}")
            print("# " + str(e))
            continue

        result = run_draw(font_path, options, text, unicodes)
        if result.returncode != 0:
            fails += 1
            print(f"not ok {number} - {name}")
            print(f"# hb-draw exited with code {result.returncode}")
            stderr = result.stderr.strip()
            if stderr:
                for l in stderr.splitlines():
                    print("# " + l)
            continue

        with open(expected_path, encoding="utf-8") as fp:
            expected = fp.read()
        actual = result.stdout

        if actual == expected:
            print(f"ok {number} - {name}")
            continue

        fails += 1
        print(f"not ok {number} - {name}")
        diff = unified_diff(
            expected.splitlines(keepends=True),
            actual.splitlines(keepends=True),
            fromfile=expected_path,
            tofile="actual",
        )
        for diff_line in diff:
            print("# " + diff_line.rstrip("\n"))

    if f is not sys.stdin:
        f.close()

print(f"1..{number}")

if fails:
    sys.exit(1)
