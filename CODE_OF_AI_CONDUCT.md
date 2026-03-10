# Code of AI Conduct

This project accepts AI-assisted contributions. Before review, a human
contributor is expected to take full responsibility for the changes they
submit. If a maintainer chooses to accept a machine-originated change directly,
that maintainer takes the same responsibility for it.

## Requirements

- Use AI as a tool, not as an authority. Verify behavior, correctness, and
  compatibility yourself before submitting changes.
- Keep changes narrow. Do not use AI to generate broad rewrites, drive-by
  refactors, or style-only churn.
- Preserve HarfBuzz's public API and ABI unless a change explicitly calls for
  it.
- Follow the repository guidance in [`AGENTS.md`](AGENTS.md),
  [`README.md`](README.md), [`BUILD.md`](BUILD.md), [`TESTING.md`](TESTING.md),
  and related local documentation.
- Do not submit generated code, text, or tests that you do not understand.
- Do not fabricate benchmarks, bug reports, test results, or reproduction
  steps.
- Do not paste private code, non-public fonts, credentials, tokens, or other
  confidential material into external AI systems.
- Respect licensing and attribution requirements. Do not submit AI-generated
  content that may have unclear provenance or incompatible licensing.

## Attribution

- When AI tools contributed meaningfully to a change, add an `Assisted-by:`
  trailer to the commit message (e.g. `Assisted-by: Claude`). Routine use of
  autocompletion or spelling correction does not require attribution.

## Pull requests

- Describe what was verified locally, including the exact build or test commands
  you ran.
- Call out uncertainty, skipped validation, or areas that need extra review.
- Be prepared to revise or discard AI-generated changes that do not meet the
  project's standards.

## Maintainer expectations

- Review AI-assisted contributions by the same technical standards as any other
  contribution.
- Prefer reproducible fixes, focused diffs, and adequate tests over volume.
- Reject changes that appear to be unreviewed AI output, including changes that
  add unnecessary complexity, invent behavior, or ignore repository guidance.

In short: if AI helped produce a change, a human contributor must still own the
result end to end.
