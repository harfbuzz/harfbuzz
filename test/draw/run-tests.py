#!/usr/bin/env python3

import os
import re
import shlex
import subprocess
import sys
import tempfile
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


def open_draw_batch_process():
    cmd = [hb_draw, "--batch"]
    if EXE_WRAPPER:
        cmd = [EXE_WRAPPER] + cmd

    process = subprocess.Popen(
        cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        env=env,
        text=True,
    )
    return [process]


def close_draw_batch_process(draw_process):
    process = draw_process[0]
    if process is None:
        return

    if process.stdin:
        process.stdin.close()
    if process.stdout:
        process.stdout.close()

    process.wait()
    draw_process[0] = None


draw_process = open_draw_batch_process()


def build_draw_command(font_path, options, unicodes, output_path):
    command = []
    if options:
        command.extend(shlex.split(options))
    command.extend([f"--output-file={output_path}", f"--unicodes={unicodes}", font_path])
    return command


def run_draw_single(command):
    cmd = [hb_draw]
    cmd.extend(command)
    if EXE_WRAPPER:
        cmd = [EXE_WRAPPER] + cmd
    return subprocess.run(cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)


def draw_cmd(command, draw_process):
    process = draw_process[0]

    # (Re)start drawer if it is dead.
    if process.poll() is not None:
        if process.stdin:
            process.stdin.close()
        if process.stdout:
            process.stdout.close()
        draw_process[0] = open_draw_batch_process()[0]
        process = draw_process[0]

    process.stdin.write(";".join(command) + "\n")
    process.stdin.flush()

    diagnostics = []
    while True:
        line = process.stdout.readline()
        if line == "":
            return None, diagnostics

        line = line.rstrip("\n")
        if line in ("success", "failure"):
            return line, diagnostics

        diagnostics.append(line)


def run_draw(font_path, options, unicodes, output_path):
    command = build_draw_command(font_path, options, unicodes, output_path)
    status, diagnostics = draw_cmd(command, draw_process)

    if status == "success":
        return subprocess.CompletedProcess(
            args=command,
            returncode=0,
            stdout="",
            stderr="\n".join(diagnostics),
        )

    # Fall back to one-shot execution for proper diagnostics.
    result = run_draw_single(command)
    if diagnostics:
        stderr = "\n".join(diagnostics)
        if result.stderr:
            stderr += "\n" + result.stderr
        result = subprocess.CompletedProcess(
            args=result.args,
            returncode=result.returncode,
            stdout=result.stdout,
            stderr=stderr,
        )
    return result


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

try:
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
                parse_unicodes(unicodes)
            except ValueError as e:
                fails += 1
                print(f"not ok {number} - {name}")
                print("# " + str(e))
                continue

            with tempfile.NamedTemporaryFile(prefix="hb-draw-test-", suffix=".svg", delete=False) as tmp:
                output_path = tmp.name

            try:
                result = run_draw(font_path, options, unicodes, output_path)
                if result.returncode != 0:
                    fails += 1
                    print(f"not ok {number} - {name}")
                    print(f"# hb-draw exited with code {result.returncode}")
                    stderr = result.stderr.strip()
                    if stderr:
                        for l in stderr.splitlines():
                            print("# " + l)
                    continue

                if not os.path.exists(output_path):
                    fails += 1
                    print(f"not ok {number} - {name}")
                    print("# hb-draw did not create output file")
                    continue

                with open(expected_path, encoding="utf-8") as fp:
                    expected = fp.read()
                with open(output_path, encoding="utf-8") as fp:
                    actual = fp.read()

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
            finally:
                if os.path.exists(output_path):
                    os.unlink(output_path)

        if f is not sys.stdin:
            f.close()
finally:
    close_draw_batch_process(draw_process)

print(f"1..{number}")

if fails:
    sys.exit(1)
