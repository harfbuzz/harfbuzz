#!/bin/bash

set -euo pipefail

usage() {
  cat <<EOF
Usage: $0 [-o TESTS_FILE] [-e EXPECTED_FILE] [--expected-dir DIR] [--subset-dir DIR] [--hb-shape PATH] [--hb-view PATH] HB_SUBSET HB_DRAW FONT_FILE [DRAW_OPTIONS] [TEXT]

Subset font, verify visual equivalence with hb-view, and append draw test line:
  font-file;options;text;expected-file

Options:
  -o TESTS_FILE       Append test line to this file (default: stdout).
  -e EXPECTED_FILE    Expected output file path (default: hash-based name).
  --expected-dir DIR  Directory for auto-generated expected files, relative
                      to TESTS_FILE directory (default: ../expected).
  --subset-dir DIR    Directory for subset fonts, relative to TESTS_FILE
                      directory (default: ../fonts).
  --hb-shape PATH     Path to hb-shape (default: derived from HB_DRAW).
  --hb-view PATH      Path to hb-view (default: derived from HB_DRAW).
EOF
}

sha1_file() {
  if [ "$SHA1SUM_KIND" = "sha1sum" ]; then
    sha1sum "$1" | cut -d' ' -f1
  elif [ "$SHA1SUM_KIND" = "shasum" ]; then
    shasum -a 1 "$1" | cut -d' ' -f1
  else
    digest -a sha1 "$1" | cut -d' ' -f1
  fi
}

sha1_stdin() {
  if [ "$SHA1SUM_KIND" = "sha1sum" ]; then
    sha1sum | cut -d' ' -f1
  elif [ "$SHA1SUM_KIND" = "shasum" ]; then
    shasum -a 1 | cut -d' ' -f1
  else
    digest -a sha1 | cut -d' ' -f1
  fi
}

is_draw_only_option() {
  case "$1" in
    -q|--quiet|--ned|--precision|--precision=*|-o|--output-file|--output-file=*|-O|--output-format|--output-format=*)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

if which sha1sum >/dev/null 2>/dev/null; then
  SHA1SUM_KIND=sha1sum
elif which shasum >/dev/null 2>/dev/null; then
  SHA1SUM_KIND=shasum
elif which digest >/dev/null 2>/dev/null; then
  SHA1SUM_KIND=digest
else
  echo "'sha1sum' not found" >&2
  exit 2
fi

out=/dev/stdout
expected_file=
expected_dir=../expected
subset_dir=../fonts
hb_shape=
hb_view=

while [ $# -gt 0 ]; do
  case "$1" in
    -o)
      out=$2
      shift 2
      ;;
    -e|--expected-file)
      expected_file=$2
      shift 2
      ;;
    --expected-dir)
      expected_dir=$2
      shift 2
      ;;
    --subset-dir)
      subset_dir=$2
      shift 2
      ;;
    --hb-shape)
      hb_shape=$2
      shift 2
      ;;
    --hb-view)
      hb_view=$2
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      break
      ;;
    *)
      break
      ;;
  esac
done

if [ $# -lt 3 ]; then
  usage >&2
  exit 1
fi

hb_subset=$1
shift
hb_draw=$1
shift
fontfile=$1
shift

if [ "x${fontfile:0:1}" = "x-" ]; then
  echo "Specify font file before draw options." >&2
  exit 1
fi
if [ ! -f "$fontfile" ]; then
  echo "Font file not found: '$fontfile'." >&2
  exit 1
fi
if ! echo "$hb_subset" | grep -q 'subset'; then
  echo "Specify hb-subset as first positional argument: got '$hb_subset'." >&2
  exit 1
fi
if ! echo "$hb_draw" | grep -q 'draw'; then
  echo "Specify hb-draw as second positional argument: got '$hb_draw'." >&2
  exit 1
fi

if [ -z "$hb_shape" ]; then
  hb_shape=$(echo "$hb_draw" | sed 's/hb-draw/hb-shape/g')
fi
if [ -z "$hb_view" ]; then
  hb_view=$(echo "$hb_draw" | sed 's/hb-draw/hb-view/g')
fi

options=()
shape_view_options=()
have_text=false
text=
for arg in "$@"; do
  if ! $have_text && [ "x${arg:0:1}" = "x-" ]; then
    if echo "$arg" | grep -q ' '; then
      echo "Space in argument is not supported: '$arg'." >&2
      exit 1
    fi
    options+=("$arg")
    if ! is_draw_only_option "$arg"; then
      shape_view_options+=("$arg")
    fi
    continue
  fi

  if $have_text; then
    echo "Too many arguments found. Use '=' notation for options, eg --font-size=64." >&2
    exit 1
  fi
  text=$arg
  have_text=true
done

if ! $have_text; then
  text=$(cat)
fi

if [[ "$text" == *';'* ]]; then
  echo "Text cannot contain ';' in .tests format." >&2
  exit 1
fi
if [[ "$text" == *$'\n'* ]]; then
  echo "Multiline text is not supported by .tests format." >&2
  exit 1
fi

options_str=
for arg in "${options[@]}"; do
  options_str="${options_str}${options_str:+ }${arg}"
done

ext=svg
for arg in "${options[@]}"; do
  if [ "x$arg" = "x-q" ] || [ "x$arg" = "x--quiet" ]; then
    ext=txt
    break
  fi
done

if [ "$out" = "/dev/stdout" ] || [ "$out" = "-" ]; then
  out_dir=$PWD
else
  mkdir -p "$(dirname "$out")"
  out_dir=$(cd "$(dirname "$out")" && pwd)
fi

tmpdir=$(mktemp -d)

# Collect gids from shaping result and subset.
glyph_ids=$(
  printf '%s\n' "$text" | "$hb_shape" "${shape_view_options[@]}" --no-glyph-names --no-clusters --no-positions "$fontfile" |
  sed 's/[][]//g; s/|/,/g'
)
cp "$fontfile" "$tmpdir/font.ttf"
"$hb_subset" \
  --glyph-names \
  --no-hinting \
  --layout-features='*' \
  --gids="$glyph_ids" \
  --text="$text" \
  --output-file="$tmpdir/font.subset.ttf" \
  "$tmpdir/font.ttf"
if ! test -s "$tmpdir/font.subset.ttf"; then
  echo "Subsetter did not produce nonempty subset font in $tmpdir/font.subset.ttf." >&2
  exit 2
fi

# Verify visual parity via hb-view.
printf '%s\n' "$text" | "$hb_view" "${shape_view_options[@]}" "$fontfile" --output-format=png --output-file="$tmpdir/orig.png"
printf '%s\n' "$text" | "$hb_view" "${shape_view_options[@]}" "$tmpdir/font.subset.ttf" --output-format=png --output-file="$tmpdir/subset.png"
if ! cmp -s "$tmpdir/orig.png" "$tmpdir/subset.png"; then
  echo "Subset font produced different rendering. Please inspect $tmpdir/orig.png and $tmpdir/subset.png." >&2
  exit 2
fi

if [ "x${subset_dir:0:1}" = "x/" ]; then
  subset_dir_path="$subset_dir"
else
  subset_dir_path="$out_dir/$subset_dir"
fi
mkdir -p "$subset_dir_path"
subset_hash=$(sha1_file "$tmpdir/font.subset.ttf")
subset_path="$subset_dir_path/$subset_hash.ttf"
if ! test -f "$subset_path"; then
  mv "$tmpdir/font.subset.ttf" "$subset_path"
fi

if [ -z "$expected_file" ]; then
  expected_hash=$(
    printf '%s;%s;%s' "$subset_hash" "$options_str" "$text" | sha1_stdin
  )
  expected_file="$expected_dir/$expected_hash.$ext"
fi
if [ "x${expected_file:0:1}" = "x/" ]; then
  expected_path="$expected_file"
else
  expected_path="$out_dir/$expected_file"
fi

mkdir -p "$(dirname "$expected_path")"
"$hb_draw" "${options[@]}" "$subset_path" "$text" > "$expected_path"

if [ "$out" = "/dev/stdout" ] || [ "$out" = "-" ]; then
  font_field=$(/usr/bin/env python3 -c 'import os,sys; print(os.path.relpath(sys.argv[1], sys.argv[2]))' "$subset_path" "$PWD")
  expected_field=$(/usr/bin/env python3 -c 'import os,sys; print(os.path.relpath(sys.argv[1], sys.argv[2]))' "$expected_path" "$PWD")
  out_file=/dev/stdout
else
  font_field=$(/usr/bin/env python3 -c 'import os,sys; print(os.path.relpath(sys.argv[1], sys.argv[2]))' "$subset_path" "$out_dir")
  expected_field=$(/usr/bin/env python3 -c 'import os,sys; print(os.path.relpath(sys.argv[1], sys.argv[2]))' "$expected_path" "$out_dir")
  out_file="$out"
fi

echo "${font_field};${options_str};${text};${expected_field}" >> "$out_file"

rm -rf "$tmpdir"
