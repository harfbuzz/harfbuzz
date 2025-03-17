#!/usr/bin/env python3

import sys, os, subprocess, hashlib

args = sys.argv[1:]

if not args or args[0].find("hb-shape") == -1 or not os.path.exists(args[0]):
    sys.exit("""First argument does not seem to point to usable hb-shape.""")
hb_shape, args = args[0], args[1:]

env = os.environ.copy()
env["LC_ALL"] = "C"


def open_shape_batch_process():
    global hb_shape, env
    process = subprocess.Popen(
        [hb_shape, "--batch"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=sys.stdout,
        env=env,
    )
    return process


shape_process = open_shape_batch_process()
no_glyph_names_process = None


def shape_cmd(command, shape_process, verbose=True):
    global hb_shape

    # (Re)start shaper if it is dead
    if shape_process.poll() is not None:
        shape_process = open_shape_batch_process()

    if verbose:
        print(hb_shape + " " + " ".join(command))
    shape_process.stdin.write((";".join(command) + "\n").encode("utf-8"))
    shape_process.stdin.flush()
    return shape_process.stdout.readline().decode("utf-8").strip()


def plural(what):
    if not what.endswith("s"):
        what += "s"
    return what


def whats_var_name(what):
    return plural(what).replace("-", "_")


def supported_whats_var_name(what):
    whats = whats_var_name(what)
    return "supported_" + whats


def supported_whats(what):
    return globals()[supported_whats_var_name(what)]


# Collect supported backends
for what in ["shaper", "face-loader", "font-funcs"]:
    subcommand = "--list-" + plural(what)

    what_process = subprocess.Popen(
        [hb_shape, subcommand],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=sys.stdout,
        env=env,
    )
    # Capture the output
    what_list = what_process.communicate()[0].decode("utf-8").strip().split()
    print(what, end=": ")
    print(what_list)
    var_name = supported_whats_var_name(what)
    globals()[var_name] = what_list

passes = 0
fails = 0
skips = 0

if not len(args):
    args = ["-"]

for filename in args:
    if filename == "-":
        print("Running tests from standard input")
    else:
        print("Running tests in " + filename)

    if filename == "-":
        f = sys.stdin
    else:
        f = open(filename, encoding="utf8")

    for line in f:
        comment = False
        if line.startswith("#"):
            comment = True
            line = line[1:]

            if line.startswith(" "):
                print("#%s" % line)
                continue

        line = line.strip()
        if not line:
            continue

        fontfile, options, unicodes, glyphs_expected = line.split(";")
        options = options.split()
        if fontfile.startswith("/") or fontfile.startswith('"/'):
            if os.name == "nt":  # Skip on Windows
                continue

            fontfile, expected_hash = (fontfile.split("@") + [""])[:2]

            try:
                with open(fontfile, "rb") as ff:
                    if expected_hash:
                        actual_hash = hashlib.sha1(ff.read()).hexdigest().strip()
                        if actual_hash != expected_hash:
                            print(
                                "different version of %s found; Expected hash %s, got %s; skipping."
                                % (fontfile, expected_hash, actual_hash)
                            )
                            skips += 1
                            continue
            except IOError:
                print("%s not found, skip." % fontfile)
                skips += 1
                continue
        else:
            cwd = os.path.dirname(filename)
            fontfile = os.path.normpath(os.path.join(cwd, fontfile))

        if comment:
            print('# %s "%s" --unicodes %s' % (hb_shape, fontfile, unicodes))
            continue

        skip_test = False
        shaper = ""
        face_loader = ""
        font_funcs = ""
        for what in ["shaper", "face-loader", "font-funcs"]:
            it = iter(options)
            for option in it:
                if option.startswith("--" + what):
                    try:
                        backend = option.split("=")[1]
                    except IndexError:
                        backend = next(it)
                    if backend not in supported_whats(what):
                        skips += 1
                        print(f"Skipping test with {what}={backend}.")
                        skip_test = True
                        break
                    what = what.replace("-", "_")
                    globals()[what] = backend
            if skip_test:
                break
        if skip_test:
            continue

        for font_funcs in [None] if font_funcs else supported_font_funcs:

            extra_options = []

            if font_funcs:
                extra_options.append("--font-funcs=" + font_funcs)

            if glyphs_expected != "*":
                extra_options.append("--verify")
                extra_options.append("--unsafe-to-concat")

            print(
                "# shaper=%s face-loader=%s font-funcs=%s"
                % (shaper, face_loader, font_funcs)
            )
            cmd = [fontfile] + ["--unicodes", unicodes] + options + extra_options
            glyphs = shape_cmd(cmd, shape_process).strip()

            if glyphs_expected == "*":
                passes += 1
                continue

            if glyphs != glyphs_expected and glyphs.find("gid") != -1:
                if not no_glyph_names_process:
                    no_glyph_names_process = open_shape_batch_process()

                cmd = [fontfile] + ["--glyphs", "--no-glyph-names", glyphs]
                glyphs = shape_cmd(cmd, no_glyph_names_process, verbose=False).strip()

                cmd = [fontfile] + ["--glyphs", "--no-glyph-names", glyphs_expected]
                glyphs_expected = shape_cmd(
                    cmd, no_glyph_names_process, verbose=False
                ).strip()

            if glyphs != glyphs_expected:
                print(" ".join(cmd), file=sys.stderr)
                print("Actual:   " + glyphs, file=sys.stderr)
                print("Expected: " + glyphs_expected, file=sys.stderr)
                fails += 1
            else:
                passes += 1

print(
    "%d tests passed; %d failed; %d skipped." % (passes, fails, skips), file=sys.stderr
)
if not (fails + passes):
    print("No tests ran.")
elif not (fails + skips):
    print("All tests passed.")

if fails:
    sys.exit(1)
elif passes:
    sys.exit(0)
else:
    sys.exit(77)
