#!/usr/bin/env python3

import argparse
import math
import os
from pathlib import Path
import re
import subprocess
import time


def prepare_cache(args):
    stats = subprocess.check_output([args.variant, "-s"], text=True)
    print(stats, end="" if stats.endswith("\n") else "\n")

    if args.variant == "ccache":
        counters = subprocess.check_output(["ccache", "--print-stats"], text=True)
        match = re.search(r"^cache_miss\s+(\d+)\s*$", counters, re.MULTILINE)
    else:
        match = re.search(r"^Cache misses\s+(\d+)\s*$", stats, re.MULTILINE)

    if match is None:
        raise ValueError("could not read the compiler cache miss count")
    if int(match.group(1)) == 0:
        print("No compiler cache misses; keeping the existing snapshot.")
        return {"save": "false"}

    workspace = Path(os.environ["GITHUB_WORKSPACE"])
    variable = "CCACHE_DIR" if args.variant == "ccache" else "SCCACHE_DIR"
    cache_location = os.environ.get(variable) or str(workspace / ("." + args.variant))
    cache_path = Path(cache_location)

    if args.variant == "ccache":
        # Keep objects accessed after setup began, including restored cache hits.
        age = max(1, math.ceil(time.time() - args.start))
        subprocess.check_call(["ccache", "--evict-older-than", str(age) + "s"])

    if not cache_path.is_dir() or not any(p.is_file() for p in cache_path.rglob("*")):
        print("No compiler cache files to save.")
        return {"save": "false"}

    key = "{}-{}-{}-{}".format(
        args.variant, args.key, os.environ["GITHUB_RUN_ID"], os.environ["GITHUB_RUN_ATTEMPT"]
    )
    return {"save": "true", "path": cache_location, "key": key}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--key", required=True)
    parser.add_argument("--variant", required=True, choices=["ccache", "sccache"])
    parser.add_argument("--start", required=True, type=float)
    args = parser.parse_args()

    try:
        outputs = prepare_cache(args)
    except Exception as error:
        print("::warning::Skipping compiler cache save: {}".format(error))
        outputs = {"save": "false"}

    try:
        with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as output:
            for name, value in outputs.items():
                output.write("{}={}\n".format(name, value))
    except Exception as error:
        print("::warning::Could not write compiler cache outputs: {}".format(error))


if __name__ == "__main__":
    main()
