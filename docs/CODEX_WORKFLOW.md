# Operating Codex for DiskSan v0.3.0

## Setup - merge; do not replace the project

Install `AGENTS.md` at the DiskSan repository root and each file in this kit's `docs` folder into the matching existing `DiskSan/docs` location. Keep the nested `docs/specs/v0.3.0.md` path. Do not create `docs/docs` and do not remove any existing 0.2.0 files. Files named `docs/CODEX_GOAL.md`, `docs/CODEX_WORKFLOW.md` and `AGENTS.md` replace the prior v0.2.0 task-oriented instructions after reviewing local custom edits. The other newly versioned files are additive.

```
DiskSan/
|-- AGENTS.md                         # Updated stable instructions
|-- CMakeLists.txt
|-- src/
|-- include/
|-- tests/
|-- docs/
|   |-- CODEX_GOAL.md                 # Updated new objective
|   |-- CODEX_WORKFLOW.md             # Updated operator guidance
|   |-- FILE_MANIFEST.txt             # Merge/check current actual files
|   |-- PROGRESS.md                   # Retain old v0.2.0 history
|   |-- BASELINE_0.2.0.md
|   |-- PROGRESS_0.3.0.md
|   |-- PROTOCOL_QUERY_AUDIT_0.3.0.md
|   |-- PROTOCOL_TEST_MATRIX_0.3.0.md
|   |-- ACCEPTANCE_0.3.0.md
|   |-- RELEASE_CHECKLIST_0.3.0.md
|   |-- ROADMAP_TO_1.0.0.md
|   `-- specs/
|       |-- v0.2.0.md                # Retain previous spec
|       `-- v0.3.0.md                # New authoritative spec
`-- tools/
```

## Preflight

From the repository root:

```sh
git status --short
git branch --show-current
git rev-parse HEAD
python3 tools/check_documentation.py
cmake -S . -B build-v030-baseline -DCMAKE_BUILD_TYPE=Release
cmake --build build-v030-baseline --config Release
ctest --test-dir build-v030-baseline -C Release --output-on-failure
```

If the documentation checker reports a manifest discrepancy, reconcile `docs/FILE_MANIFEST.txt` with the *live* maintained source files; do not delete newer source just to match this reference manifest. If the checkout has newer commits, inspect differences rather than reset. The v0.2.0 release SHA is a historical reference only.

Windows multi-config builds should use `-C Release`. Linux requires development libudev/pkg-config, and tests require Python. Run the sanitizer build on actual supported hosts. Avoid blanket `ctest` skips and retry workarounds that hide real failures.

## Start the implementation goal

Use GPT-6 Astra with a high thinking effort for native query safety and parser contracts. In a Codex interface exposing Goals, copy only the single line in the `Ready-to-paste /goal` section of `docs/CODEX_GOAL.md`, not the whole Markdown file or merely its path. If the client does not support Goals, send the ordinary fallback prompt.

Do not tell Codex to begin by downloading a tagged release ZIP: the local checkout is the source of changes. Do not let a parallel agent edit the same public header/assessment model as the primary implementation thread. Parallel research or independent fixture drafting is fine only if the changes are reconciled.

## Working method / token use

For each milestone: read its relevant version-spec section, the query-audit row and affected source/tests; implement the smallest cohesive change; run focused tests; fix root causes; then run wider regression before marking PASS. Preserve the old `docs/PROGRESS.md` untouched and update only `docs/PROGRESS_0.3.0.md` with source SHA, commands, actual results and skipped external checks. Do not flood context with huge CI logs or complete inventories.

No automatic or scheduled command should probe actual storage hardware. The unit/integration tests use fake request wrappers and fixtures. An explicit maintainer-chosen physical NVMe query is a separate, logged verification step, not a prerequisite to every iterative build.

## Minimum test/verification commands

```sh
cmake -S . -B build-v030 -DCMAKE_BUILD_TYPE=Release -DDISKSAN_WERROR=ON
cmake --build build-v030 --config Release
ctest --test-dir build-v030 -C Release --output-on-failure
python3 tools/check_documentation.py
git diff --check
```

When a supported native sanitizer toolchain is available:

```sh
cmake -S . -B build-v030-sanitize -DCMAKE_BUILD_TYPE=Debug -DDISKSAN_SANITIZERS=ON
cmake --build build-v030-sanitize
ctest --test-dir build-v030-sanitize --output-on-failure
```

Do not claim a GitHub CI run passed until a commit is actually pushed by the maintainer and the matching workflow run is observed. Never stage, push or trigger publication without permission.

## Before maintainer release

Use `docs/ACCEPTANCE_0.3.0.md`, `docs/PROTOCOL_TEST_MATRIX_0.3.0.md` and `docs/RELEASE_CHECKLIST_0.3.0.md` as acceptance references. Update product docs, code contracts, changelog and roadmap as needed; do not delete archived documents. Confirm exact compiler/test provenance, native OS query coverage, and physical-device gaps before calling it releasable. Rebuild final archives/checksums after every late source or packaged-document change. Report an honest unsupported-platform scope rather than invent uniform OS coverage.

The assistant may prepare local release files, but the maintainer decides when to commit, tag `v0.3.0` and create/publish a pre-release.
