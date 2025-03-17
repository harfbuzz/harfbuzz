#!/usr/bin/env python3

import sys, os, subprocess, hashlib


def shape_cmd(command):
    global hb_shape, process
    print(hb_shape + " " + " ".join(command))
    process.stdin.write((";".join(command) + "\n").encode("utf-8"))
    process.stdin.flush()
    return process.stdout.readline().decode("utf-8").strip()


args = sys.argv[1:]

backends = []
if os.getenv("HAVE_FREETYPE", "1") == "1":
    backends.append("ft")
if os.getenv("HAVE_FONTATIONS", "0") == "1":
    backends.append("fontations")
if os.getenv("HAVE_CORETEXT", "0") == "1":
    backends.append("coretext")
if os.getenv("HAVE_DIRECTWRITE", "0") == "1":
    backends.append("directwrite")
if os.getenv("HAVE_UNISCRIBE", "0") == "1":
    backends.append("uniscribe")

have_freetype = int(os.getenv("HAVE_FREETYPE", 1))

if not args or args[0].find("hb-shape") == -1 or not os.path.exists(args[0]):
    sys.exit("""First argument does not seem to point to usable hb-shape.""")
hb_shape, args = args[0], args[1:]

env = os.environ.copy()
env["LC_ALL"] = "C"
process = subprocess.Popen(
    [hb_shape, "--batch"],
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=sys.stdout,
    env=env,
)

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

        extra_options = ["--shaper=ot"]
        if glyphs_expected != "*":
            extra_options.append("--verify")
            extra_options.append("--unsafe-to-concat")

        if comment:
            print('# %s "%s" --unicodes %s' % (hb_shape, fontfile, unicodes))
            continue

        skip_test = False
        shaper = ""
        face_loader = ""
        font_funcs = ""
        for what in ["--shaper", "--face-loader", "--font-funcs"]:
            it = iter(options)
            for option in it:
                if option.startswith(what):
                    try:
                        backend = option.split("=")[1]
                    except IndexError:
                        backend = next(it)
                    if backend not in backends:
                        skips += 1
                        print("Skipping test with ${what}=${backend}.")
                        skip_test = True
                        break
                    what = what[2:].replace("-", "_")
                    globals()[what] = backend
            if skip_test:
                break
        if skip_test:
            continue

        print(
            "shaper=%s face_loader=%s font_funcs=%s" % (shaper, face_loader, font_funcs)
        )

        if "--font-funcs=ot" in options or not have_freetype:
            glyphs1 = shape_cmd(
                [fontfile, "--font-funcs=ot"]
                + extra_options
                + ["--unicodes", unicodes]
                + options
            )
        else:
            glyphs1 = shape_cmd(
                [fontfile, "--font-funcs=ft"]
                + extra_options
                + ["--unicodes", unicodes]
                + options
            )
            glyphs2 = shape_cmd(
                [fontfile, "--font-funcs=ot"]
                + extra_options
                + ["--unicodes", unicodes]
                + options
            )

            if glyphs1 != glyphs2 and glyphs_expected != "*":
                print("FT funcs: " + glyphs1, file=sys.stderr)
                print("OT funcs: " + glyphs2, file=sys.stderr)
                fails += 1
            else:
                passes += 1

        if glyphs1.strip() != glyphs_expected and glyphs_expected != "*":
            print("hb-shape", fontfile, "--unicodes", unicodes, file=sys.stderr)
            print("Actual:   " + glyphs1, file=sys.stderr)
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
