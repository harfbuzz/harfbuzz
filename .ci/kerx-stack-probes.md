# CoreText compatibility for `kerx` format 1

HarfBuzz and HarfRust emulate native CoreText's format 1 history behavior,
which differs from [Apple's written stack-pop rules][spec]. The native
measurements below cover 35 cases, including both Arabic examples from
[the original report][report].

## The discrepancy and compatibility rules

The written rules describe an eight-entry glyph stack, a value-array index,
a destructive pop per value or tuple, and a separate `0xFFFF` terminator.
The reset flag is described as clearing the stack; its execution order is
not specified.

Native CoreText instead behaves as follows in the measured cases:

- Keep eight circular history slots. Slots initially refer to the
  first glyph of the run. Each push replaces the next slot and advances the
  write cursor.
- For every action list, start a separate read cursor at the write cursor.
  Walk backwards through the slots, wrapping every eight values. Applying a
  value does not remove the entry from history.
- A zero value still advances the read cursor. Duplicate pushes are visited
  separately. `0xFFFF` terminates the list without visiting a slot.
- Ignore the reset flag. Long action lists after resets, and a history probe
  with a reset on every push, produce the same history as without resets.

Thus short lists can reuse entries adjusted by previous lists. With fewer
than eight pushes, unused slots still refer to the first glyph of the run.
With more than eight pushes, older glyphs are overwritten. Long lists cycle
through those eight slots, rather than stopping or indefinitely clamping to
the oldest glyph.

This is an implementation model supported by the recorded probes, rather
than a claim about CoreText's internal data structures.

HarfBuzz applies these rules only to extended `kerx` format 1; legacy
`kern` retains its existing pop and termination rules. HarfRust's legacy
`kern` implementation is likewise unchanged. Both implementations use
fixed-size storage and charge action-list traversal to the operation budget.
Bounds checks and the budget limit still apply to unterminated lists.

The value-index, separate terminator, and `processDirection` fixes in
HB [#6538][hb-fix] and HR [#572][hr-values]/[#575][hr-direction] are separate
from this compatibility change. Fontations' value-array accessor needs no
change for history execution.

## Concrete differences from the previous implementation

The `retained-stack` probe uses
`cc3a5bc0b7af5ea992fa8aa7810973e294d3b381.ttf` and text `aab`.
After three pushes, the first list applies `31`; the later list contains
`13, 17, 19, 23`. Both lists end in `0xFFFF`.

| Measurement | Previous HB / HR | CoreText and updated HB / HR |
| --- | --- | --- |
| Per-glyph adjustments | `17, 13, 31` | `42, 17, 44` |
| Absolute x positions | `17, 591, 1183` | `42, 620, 1225` |

The later list revisits the newest glyph (`31 + 13 = 44`), then the middle
glyph (`17`), then the oldest twice (`19 + 23 = 42`).

The `history-20` probe pushes each glyph and immediately applies `10`,
then applies values `1` through `21` at end of text. Its additional
adjustments, in glyph order, are:

```
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 22, 20, 39, 36, 33, 30, 27
```

History-length probes at 1, 2, 3, 7, 8, 9, 16, and 17 pushes verify the
wraparound and initial slots. A non-pushed `b` before the first push of `a`
shows that unused slots still refer to the run's first glyph: a list of 64
ones adjusts `b` by 56 and `a` by 8. Reset probes cover repeated resets,
a reset without a push, actions before the first push, and long-list reuse.
The `reset-without-push` case adjusts the previous pushed `a` by a total
of `50`; it does not transfer the history to the current `b`.

Both the original and revised Arabic examples now have the same glyph
positions and final extent as CoreText and their GPOS reference. This
includes the original example's lam adjustment that destructive popping
missed.

## Evidence and reproduction

Native measurements were taken on 2026-11-01 at commit
`7dd5c945da609998a9840d37c5635dec499af622`:

- [macOS 15.8.2, build 24H232][mac15].
- [macOS 26.6.2, build 25G83][mac26].

All 35 cases produced identical glyph records on the two systems.
[kerx-stack-results.json](kerx-stack-results.json) preserves the records;
[kerx-stack-probes.json](kerx-stack-probes.json) maps case names to fonts and
input text. The shaping regressions in HB and HR cover these same inputs.
The native probes can be rerun manually when investigating CoreText changes.

On macOS, from the repository root:

```sh
clang -std=c11 -Wall -Wextra -Werror .ci/kerx-stack-probe.c \
  -framework CoreFoundation -framework CoreGraphics -framework CoreText \
  -o /tmp/kerx-stack-probe
python3 - <<'PY'
import json
import subprocess

with open(".ci/kerx-stack-probes.json") as source:
    cases = json.load(source)
for case in cases:
    raw = subprocess.check_output(
        ["/tmp/kerx-stack-probe", case["font"], case["text"]], text=True
    )
    print(json.dumps({"case": case["name"], "glyphs": json.loads(raw)}))
PY
```

The probe sets the CoreText font size to its units per em. Compare glyph
identities, absolute positions, and final extent. Individual advances and
offsets can distribute identical spacing differently. Use HB's
`--cluster-level=1` when comparing CoreText's character indices; HB's default
grapheme clusters merge Arabic marks with their bases.

[hb-fix]: https://github.com/harfbuzz/harfbuzz/pull/6538
[hr-values]: https://github.com/harfbuzz/harfrust/pull/572
[hr-direction]: https://github.com/harfbuzz/harfrust/pull/575
[spec]: https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6kerx.html
[report]: https://github.com/harfbuzz/harfbuzz/pull/6538#issuecomment-6561986849
[mac15]: https://github.com/harfbuzz/harfbuzz/actions/runs/41342459155/job/124955174169
[mac26]: https://github.com/harfbuzz/harfbuzz/actions/runs/41342459155/job/124955174419
