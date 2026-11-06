# DiskSan v0.4.0 - installation and Codex workflow

## Merge, do not nest or replace the repository

```
DiskSan/
  AGENTS.md                         # Replace/merge standing instructions
  docs/
    BASELINE_0.3.0.md               # NEW historical published baseline
    CODEX_GOAL.md                   # Updated to v0.4.0
    CODEX_WORKFLOW.md               # Updated to v0.4.0
    GUI_ARCHITECTURE_0.4.0.md        # NEW bridge/build design
    GUI_UX_SPEC_0.4.0.md             # NEW UI user journeys
    THREAT_MODEL_0.4.0.md            # NEW design-only safety model
    SIMULATION_CONTRACT_0.4.0.md     # NEW CLI simulation contract
    TEST_MATRIX_0.4.0.md            # NEW verification gates
    ACCEPTANCE_0.4.0.md             # NEW requirement audit
    PROGRESS_0.4.0.md               # NEW progress ledger
    RELEASE_CHECKLIST_0.4.0.md      # NEW maintainer release gates
    ROADMAP_TO_1.0.0.md             # Updated, compare first
    FILE_MANIFEST.txt               # Expected 0.3.0 tree + this kit
    specs/
      v0.4.0.md                     # NEW complete specification
```

Do not create `DiskSan/docs/docs`. The entire ZIP is a **documentation kit**, not a code patch; it does not claim to implement 0.4.0. `AGENTS.md` belongs at the repository root, all other files as indicated. Keep existing `docs/specs/v0.2.0.md`, `docs/specs/v0.3.0.md`, `docs/PROGRESS.md` and `docs/PROGRESS_0.3.0.md` intact for history.

Be especially careful with `docs/FILE_MANIFEST.txt`: the included file covers the **observed** v0.3.0 tracked tree plus these kit documents. If Codex or the maintainer already added other files, merge entries with the actual live manifest. Never drop new paths, never list ZIP names, and run `python3 tools/check_documentation.py` in the real repository. The checklist and baseline are not evidence of 0.4.0 tests.

## Operating sequence

1. Merge the package and inspect the working tree; do not overwrite user modifications without comparing.
2. Open Codex at the actual DiskSan repo root. Ensure `AGENTS.md`, `docs/specs/v0.4.0.md` and `docs/PROGRESS_0.4.0.md` exist.
3. Use `/goal` from `docs/CODEX_GOAL.md`, or its ordinary fallback if unavailable. Do not paste the entire markdown into the prompt; only the ready-to-paste objective.
4. Confirm git HEAD/branch/dirty files; run baseline tests on available OS, inspect Qt6 availability/version, current CMake and the v0.3.0 CLI surfaces.
5. Implement M1-M3 pure C model and CLI in focused units. Build forbidden-call fixtures that fail if simulation invokes actual protocol/native destructive paths. Check default v0.3.0 probe gates after integration.
6. Build the optional desktop (M4-M5) using Qt Quick and a fixed QProcess/JSON bridge. The GUI should be runnable with actual safe CLI data if Qt is available; otherwise record the exact SDK blocker, continue portable implementation and do not claim GUI validation.
7. Run full regression/fault/sanitizer/docs/packaging on accessible OS and an explicit optional-GUI smoke test. Update the progress ledger only with actual commands, counts and skips.
8. Prepare release material and identify untested native platform/physical disks/GUI deployment. Do not commit/tag/push/publish until expressly told to.

## Build and test examples

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DDISKSAN_BUILD_DESKTOP=OFF
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
python3 tools/check_documentation.py

git diff --check
```

After the new option and desktop targets have actually been implemented, on a system with a supported installed Qt SDK:

```sh
cmake -S . -B build-desktop -DCMAKE_BUILD_TYPE=Release -DDISKSAN_BUILD_DESKTOP=ON
cmake --build build-desktop --config Release
ctest --test-dir build-desktop -C Release --output-on-failure
```

These are commands to run in the future implementation; **this kit has not executed them**. Use the actual platform-specific compiler generator and sanitizer commands from current TESTING.md. Don't require Qt in the default CLI. Don't use any `sudo`, force-erasure, mount or physical-device probing in the planned verification.

## Special maintenance note

Published v0.3.0 GitHub Release prose currently includes leftover "candidate" language and an assertion that committed-source CI has not yet run; its tagged CI has now passed. Editing the existing GitHub Release description is a separate maintainer editorial action; keep it from derailing v0.4.0 engineering.
