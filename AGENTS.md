# DiskSan - Codex repository instructions

## Project

DiskSan is a C17, CMake-based, read-only storage-device discovery CLI for Linux, macOS, and Windows. The 0.2.0 objective is **read-only target assessment, policy evaluation, and evidence-based capability reporting**, not data sanitization. Keep the implementation portable at its core with platform-specific code in adapters.

## Authority and boundaries

- You may independently investigate, design, edit project files, refactor, add tests, and run ordinary read-only/disposable-fixture verification needed to complete the assigned task. Make sound engineering decisions; ask only when an actual missing permission, external dependency, or unresolved product decision blocks progress.
- Do not execute destructive storage commands or modify real block-device contents, layouts, filesystems, mounts, encryption keys, or firmware state. Do not add executable erase/sanitize/discard/format backends to 0.2.0. Do not use elevated privilege merely to make an unsafe test pass.
- Do not commit, push, create or move tags, publish releases, change repository settings, or trigger deployment/publication workflows without explicit new authorization from the maintainer. You may prepare code, workflow configurations, notes, and release artifacts locally.
- Do not claim tests were run or hardware behavior verified unless the evidence exists. Cross-compilation is not native-runtime validation. A capability hint is not confirmed command support; a discovery snapshot is never erase authorization.

## Working method

- For 0.2.0 scope and acceptance criteria, consult `docs/specs/v0.2.0.md`; for baseline assumptions consult `docs/BASELINE_0.1.1.md`. Read only the sections relevant to the active task, then inspect affected source files and tests as needed.
- Work in focused, verifiable milestones. Implement the selected milestone, run targeted tests, resolve relevant failures, then run wider regression checks at integration points and final handoff. Do not stop after producing a plan when an implementation task is authorized.
- Update `docs/PROGRESS.md` after a verified milestone or handoff, not after every tool call. Report facts, evidence, blockers, and the next action; distinguish `PASS`, `FAIL`, `SKIP`, `BLOCKED`, and `NOT_RUN`.
- Prefer existing repo conventions, current APIs, simple ownership and explicit error paths. Keep changes scoped but include necessary cross-module fixes. Parallel agents are appropriate only for independent work; do not have multiple agents edit the same core files concurrently.
- Follow `docs/CODING_STANDARD.md`: English repository content, C17, explicit ownership, Doxygen file/function `@brief`, public API contracts, bounded inputs, no speculative hardware classifications. Maintain `docs/FILE_MANIFEST.txt` for every added/removed maintained file; the documentation checker enforces it.
- Preserve existing `list`, `info`, `--json`, exit-code contracts and JSON schema 2 unless a justified, documented compatibility decision is explicitly made. New machine-readable structures must represent unknown/not checked values without false certainty.

## Verification

- Main build/test route: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`, `cmake --build build --config Release`, `ctest --test-dir build -C Release --output-on-failure`.
- Also run `python3 tools/check_documentation.py`. Use `-DDISKSAN_SANITIZERS=ON` for supported native sanitizer configurations when changing memory, parsing, or device resolution; run platform-specific integration tests only where the platform is actually accessible.
- Existing GitHub Actions is in `.github/workflows/ci.yml`; never treat workflow configuration or local YAML parsing as evidence that hosted jobs passed. Check current runner/compiler capabilities before changing CI.
- On the maintainer's Mac-to-Linux-VM workflow, treat the Mac workspace as the source of truth, sync only intended DiskSan files and verify the transfer when using the VM. Use the project's *current actual paths*; do not inherit esdumanOS paths or delete unrelated VM files. Keep logs and device inventories private.

## Review priorities

1. Failure-safe target identity and topology, especially reuse, hotplug, system/media ambiguity, and layered devices.
2. Truthful capability evidence and separation of observation, policy advice, and any future authorization.
3. Memory safety, portable platform behavior, failure propagation, stable JSON, and regression coverage.
4. Clear diagnostics, documentation, reproducible tests, and honest release gates.
