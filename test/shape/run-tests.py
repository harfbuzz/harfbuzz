#!/usr/bin/env python3

import sys, os, subprocess, hashlib

print("TAP version 14")

args = sys.argv[1:]

verbose = False
if args and args[0] == "-v":
    verbose = True
    args = args[1:]

if not args or args[0].find("hb-shape") == -1 or not os.path.exists(args[0]):
    sys.exit("""First argument does not seem to point to usable hb-shape.""")
hb_shape, args = args[0], args[1:]

env = os.environ.copy()
env["LC_ALL"] = "C"

EXE_WRAPPER = os.environ.get("MESON_EXE_WRAPPER")


def open_shape_batch_process():
    cmd = [hb_shape, "--batch"]
    if EXE_WRAPPER:
        cmd = [EXE_WRAPPER] + cmd

    process = subprocess.Popen(
        cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=sys.stdout,
        env=env,
    )
    return process


shape_process = open_shape_batch_process()
no_glyph_names_process = None


def shape_cmd(command, shape_process, verbose=False):
    global hb_shape

    # (Re)start shaper if it is dead
    if shape_process.poll() is not None:
        shape_process = open_shape_batch_process()

    if verbose:
        print("# " + hb_shape + " " + " ".join(command))
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


def all_whats_var_name(what):
    whats = whats_var_name(what)
    return "all_" + whats


def all_whats(what):
    return globals()[all_whats_var_name(what)]


# Collect supported backends
for what in ["shaper", "face-loader", "font-funcs"]:
    subcommand = "--list-" + plural(what)

    cmd = [hb_shape, subcommand]
    if EXE_WRAPPER:
        cmd = [EXE_WRAPPER] + cmd

    what_process = subprocess.Popen(
        cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=sys.stdout,
        env=env,
    )
    # Capture the output
    what_list = what_process.communicate()[0].decode("utf-8").strip().split()
    if what_process.returncode:
        sys.exit(f"Failed to run: {hb_shape} {subcommand}")
    whats = plural(what)
    var_name = supported_whats_var_name(what)
    globals()[var_name] = what_list
    print(f"# Supported {whats}: {what_list}")

# If running under Wine and not native dlls, make the respective shapers unavailable.
if os.environ.get("WINEPATH"):
    overrides = os.environ.get("WINEDLLOVERRIDES", "").lower()
    if "directwrite" in supported_shapers and overrides.find("dwrite") == -1:
        supported_shapers.remove("directwrite")
        print("# Skipping DirectWrite shaper under Wine.")
    if "uniscribe" in supported_shapers and overrides.find("usp10") == -1:
        supported_shapers.remove("uniscribe")
        print("# Skipping Uniscribe shaper under Wine.")


number = 0
passes = 0
fails = 0
skips = 0

if not len(args):
    args = ["-"]

for filename in args:
    if filename == "-":
        print("# Running tests from standard input")
    else:
        print("# Running tests in " + filename)

    if filename == "-":
        f = sys.stdin
    else:
        f = open(filename, encoding="utf8")

    # By default test all backends
    for what in ["shaper", "face-loader", "font-funcs"]:
        all_var_name = all_whats_var_name(what)
        globals()[all_var_name] = supported_whats(what)
    all_face_loaders = ["ot"]  # But only 'ot' face-loader
    all_shapers = ["ot"]  # But only 'ot' shaper

    # Right now we only test the 'ot' shaper if nothing specified,
    # but try all font-funcs unless overriden.
    # Only 'ot' face-loader is tested.

    for line in f:
        comment = False
        if line.startswith("#"):
            comment = True
            line = line[1:]

            if line.startswith(" "):
                if verbose:
                    print("#%s" % line)
                continue

        line = line.strip()
        if not line:
            continue

        if line.startswith("@"):
            # Directive
            line = line.strip()
            line = line.split("#")[0].strip()[1:]
            consumed = False
            for what in ["shaper", "face-loader", "font-funcs"]:
                whats = plural(what)
                if line.startswith(what) or line.startswith(whats):
                    command, values = line.split("=")
                    values = values.strip().split(",")

                    supported = supported_whats(what)
                    if command[-1] == "-":
                        # Exclude
                        values = [v for v in supported if v not in values]
                    else:
                        # Specify
                        values = [v for v in values if v in supported]

                    var_name = all_whats_var_name(what)
                    print(f"# Setting {whats} to test to {values}")
                    globals()[var_name] = values
                    consumed = True
            if consumed:
                print("#", line)
                continue
            else:
                print("Unrecognized directive: %s" % line, file=sys.stderr)
                sys.exit(1)

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
                            skips += 1
                            print(
                                "# Different version of %s found; Expected hash %s, got %s; skipping."
                                % (fontfile, expected_hash, actual_hash)
                            )
                            continue
            except IOError:
                skips += 1
                print("# %s not found, skip." % fontfile)
                continue
        else:
            cwd = os.path.dirname(filename)
            fontfile = os.path.normpath(os.path.join(cwd, fontfile))

        if comment:
            if verbose:
                print('# # %s "%s" --unicodes %s' % (hb_shape, fontfile, unicodes))
            continue

        skip_test = False
        shaper = None
        face_loader = None
        font_funcs = None
        new_options = []
        it = iter(options)
        for option in it:
            consumed = False
            for what in ["shaper", "face-loader", "font-funcs"]:
                if option.startswith("--" + what):
                    try:
                        backend = option.split("=")[1]
                    except IndexError:
                        backend = next(it)
                    if backend not in supported_whats(what):
                        skips += 1
                        number += 1
                        print(
                            f"ok {number} - {fontfile} # skip {what}={backend} not supported"
                        )
                        print(f"# Skipping test with {what}={backend}.")
                        skip_test = True
                        break
                    what = what.replace("-", "_")
                    globals()[what] = backend
                    consumed = True
            if not consumed:
                new_options.append(option)

            if skip_test:
                break
        if skip_test:
            continue
        options = new_options

        for face_loader in [face_loader] if face_loader else all_whats("face-loader"):
            for shaper in [shaper] if shaper else all_whats("shaper"):
                for font_funcs in (
                    [font_funcs] if font_funcs else all_whats("font-funcs")
                ):
                    number += 1
                    extra_options = []

                    if shaper:
                        extra_options.append("--shaper=" + shaper)
                    if face_loader:
                        extra_options.append("--face-loader=" + face_loader)
                    if font_funcs:
                        extra_options.append("--font-funcs=" + font_funcs)

                    if glyphs_expected != "*":
                        extra_options.append("--verify")
                        extra_options.append("--unsafe-to-concat")

                    if verbose:
                        print(
                            "# shaper=%s face-loader=%s font-funcs=%s"
                            % (shaper, face_loader, font_funcs)
                        )
                    cmd = (
                        [fontfile] + ["--unicodes", unicodes] + options + extra_options
                    )
                    glyphs = shape_cmd(cmd, shape_process, verbose).strip()

                    if glyphs_expected == "*":
                        passes += 1
                        print(f"ok {number} - {fontfile}")
                        continue

                    final_glyphs = glyphs
                    final_glyphs_expected = glyphs_expected

                    if glyphs != glyphs_expected and glyphs.find("gid") != -1:
                        if not no_glyph_names_process:
                            no_glyph_names_process = open_shape_batch_process()

                        cmd2 = [fontfile] + ["--glyphs", "--no-glyph-names", glyphs]
                        final_glyphs = shape_cmd(cmd2, no_glyph_names_process).strip()

                        cmd2 = [fontfile] + [
                            "--glyphs",
                            "--no-glyph-names",
                            glyphs_expected,
                        ]
                        final_glyphs_expected = shape_cmd(
                            cmd2, no_glyph_names_process
                        ).strip()

                    # If the removal of glyph_ids failed, fail the test.
                    # https://github.com/harfbuzz/harfbuzz/issues/5169
                    if (
                        not final_glyphs_expected
                        or final_glyphs != final_glyphs_expected
                    ):
                        fails += 1
                        cmd = hb_shape + " " + " ".join(cmd)
                        print(f"not ok {number} - {cmd}")
                        print("   ---", file=sys.stderr)
                        print('   test_file: "' + filename + '"', file=sys.stderr)
                        print('   cmd: "' + cmd + '"', file=sys.stderr)
                        print('   actual:   "' + glyphs + '"', file=sys.stderr)
                        print('   expected: "' + glyphs_expected + '"', file=sys.stderr)
                        if final_glyphs != glyphs:
                            print(
                                '   actual_gids:   "' + final_glyphs + '"',
                                file=sys.stderr,
                            )
                            print(
                                '   expected_gids: "' + final_glyphs_expected + '"',
                                file=sys.stderr,
                            )
                        print("   ...", file=sys.stderr)
                    else:
                        cmd = hb_shape + " " + " ".join(cmd)
                        passes += 1
                        print(f"ok {number} - {cmd}")

print("# %d tests passed; %d failed; %d skipped." % (passes, fails, skips))
if not (fails + passes):
    print("# No tests ran.")
elif not (fails + skips):
    print("# All tests passed.")

print("1..%d" % number)
