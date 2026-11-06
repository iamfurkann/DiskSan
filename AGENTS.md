# DiskSan - standing Codex repository instructions

## Product boundary

DiskSan is an Apache-2.0 C17 storage-discovery and read-only assessment CLI for Linux, macOS and Windows. The published 0.3.0 baseline includes an **explicitly opt-in** Linux NVMe Identify Controller capability query; it has no sanitization or user-data I/O backend. The 0.4.0 goal is **inspection-only operation planning, simulation, threat/authorization architecture, and an optional Qt Quick desktop prototype**. A simulation does not erase, authorize, or prove anything about media sanitization.

## Engineering authority

- Independently investigate, design, implement, refactor, create code/tests/docs and run bounded, safe ordinary/disposable-fixture tests. Finish reachable engineering work rather than submitting a plan only. Seek input only for genuine external/product blockers.
- Never execute or add real disk erase/sanitize/format/discard, firmware mutation, mount/unmount, key deletion, storage-user-data access, or elevation pathways in 0.4.0. No executable destructive backend, native erase ioctl, generic pass-through, command sequence, future authorization token, or simulation-to-execution bridge.
- Do not commit, push, tag, create releases, upload assets, change repository settings or launch deployments without explicit maintainer authorization. Prepare a candidate and report evidence instead.
- Preserve the current git worktree and the published 0.3.0 semantics. Recheck the actual checkout rather than blindly overwriting local changes. Never silently delete user changes, tests, old version documents or existing reports.

## Source of truth and workflow

1. Read `docs/specs/v0.4.0.md`, `docs/BASELINE_0.3.0.md` and the relevant subsections of the architecture, simulation and GUI documents. This file is standing guidance, not the full specification.
2. Start with `git status`, branch, HEAD, CMake version, installed Qt and relevant toolchains. Establish a repeatable baseline. An announced release is not proof of local source or hardware state.
3. Implement verifiable milestones; test one coherent feature at a time. Update `docs/PROGRESS_0.4.0.md` after verified milestones, preserving the historical `docs/PROGRESS.md` and `docs/PROGRESS_0.3.0.md`.
4. Keep portable C17 core distinct from native adapters, desktop Qt/C++ integration, and any future privileged operation service. No platform SDK types in portable headers. No Qt dependency in core or the standalone CLI.
5. Follow `docs/CODING_STANDARD.md`: English/ASCII maintained text, Doxygen `@file`/`@brief` on C/C++ where applicable, complete public contracts, bounded sizes, resource ownership, strict error propagation. Update sorted `docs/FILE_MANIFEST.txt` whenever a maintained file is added/removed.
6. Run actual relevant portable, native, sanitizer, fault, GUI, documentation and package tests on available systems. Mark unavailable Qt/hardware/runners `NOT_RUN` or `BLOCKED`; never call a compilation check a runtime validation.

## Invariants

- Preserve 0.3.0 `list`, `info`, `assess`, `capabilities`, `--probe` gating, schema-2 types, identity/unknown/evidence semantics, exit-code classes and tagged Linux NVMe native query allowlist. No automatic `--probe` in `plan`, `simulate` or desktop.
- Existing `erase_authorized: false`, short-lived non-atomic identity observation and uncertainty cannot be turned into executable permission by a plan, simulator or UI.
- The 0.4.0 plan describes hypothetical preconditions and blockers only. A simulated step is **never** a real submitted, accepted, completed or verified erase. Unknown -> review/incomplete, not safe. Live system/root/mounted/shared/layered hazards remain blocking.
- UI must not execute arbitrary binaries, shells or user-provided commands. If using QProcess, pin the known `disksan` executable, pass a fixed allowlisted argument vector, bound output, validate JSON and detect stale/failed processes; provide an explicit disconnected/demo state. Do not print secrets or raw disk identifiers in CI or fixture reports.
- GUI build must be optional (`DISKSAN_BUILD_DESKTOP=OFF` by default). No silent GUI install in CLI release archives. Do not assume Qt is installed or Qt deployment supports the same minimum macOS as the C CLI.
- Use synthetic device fixtures for mutation/race/permission paths; no real device manipulation even if convenient. The `capabilities --probe` physical test remains deliberate, separate and never required for a routine test run.

## Acceptance vocabulary

`PASS` means specifically executed evidence; `FAIL` attempted and failed; `SKIP` a recorded skipped case; `NOT_RUN` no evidence; `BLOCKED` an external dependency; `IMPLEMENTATION_COMPLETE` reached all verifiable scope; `READY_TO_PUBLISH` needs explicit maintainer review after release gates. Document exact SHA/source snapshot and relevant commands for every claimed gate.
