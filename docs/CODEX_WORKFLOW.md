# Codex workflow for DiskSan v0.2.0

This guide installs the accompanying documents into an **existing** DiskSan checkout. It does not replace code, automatically publish a release, or activate a Goal.

## 1. File placement

Place the files in the existing repository at these paths:

```text
DiskSan/
+-- AGENTS.md                      # Standing instructions, auto-read by supported Codex tools
+-- docs/
    +-- BASELINE_0.1.1.md          # Inspected historical source and validation state
    +-- CODEX_GOAL.md              # Copy/paste Goal and ordinary fallback prompt
    +-- CODEX_WORKFLOW.md          # This operator guide
    +-- FILE_MANIFEST.txt          # Updated to include the new maintained files
    +-- PROGRESS.md                # Evidence-backed work and handoff journal
    +-- specs/
        +-- v0.2.0.md              # Full version implementation specification
```

**Merge into the real checkout** rather than replacing the project. Compare the included `FILE_MANIFEST.txt` with the actual tree; if the tree has changed, merge the added entries into the live manifest instead of deleting existing tracked paths. Do not copy older esdumanOS directory paths or other project state. `AGENTS.md` is a standing instruction document, not a complete specification.

### Expected separation

| Document | Read frequency | Purpose |
| --- | --- | --- |
| `AGENTS.md` | Automatically/on every relevant Codex session | Stable safety, permissions, standards, verification rules |
| `docs/specs/v0.2.0.md` | Relevant sections per milestone | Functional scope, contracts, acceptance tests |
| `docs/BASELINE_0.1.1.md` | Initial baseline, or when compatibility matters | Reference to existing behavior and unverified claims |
| `docs/PROGRESS.md` | Open at startup; update after verified milestones | Handoff and implementation evidence |
| `docs/CODEX_GOAL.md` | At thread creation and if goal is reset | Persistent objective and fallback prompt |
| `docs/CODEX_WORKFLOW.md` | Setup/handoff only | Human operation of the workflow |

Avoid asking Codex to reread every entire document on each small edit. Read the relevant sections and source files. The source tree and executed tests override historical assumptions in this kit.

## 2. Start Codex

1. Open the DiskSan checkout and verify the current working tree (`git status`, branch, current commit), CMake version and reachable Linux/macOS/Windows toolchains.
2. Run the baseline tests that can actually execute on the current host. Do not describe pending Windows or hosted CI runs as passed.
3. Open a new Codex thread with **GPT-6 Astra / Very High** for architecture and complex safety decisions. A lower reasoning setting is fine for isolated documentation/build tasks; correctness and verification still govern.
4. If the Codex environment supports `/goal`, copy the one-line command from `docs/CODEX_GOAL.md`. Otherwise use the ordinary fallback prompt there.
5. Let Codex work through the defined milestones, finishing and testing a cohesive unit before starting unrelated implementation.
6. Retain maintainer authority over Git commits, pushes, tags, GitHub Releases and repository settings.

The Codex `/goal` command is **a UI/CLI action entered in the thread**. Simply creating a `GOALS.md` or pasting a goal inside a code file will not activate it. Check `codex --version` in the CLI; availability depends on the Codex build/interface, not on the chosen reasoning setting.

## 3. Working cycle and token efficiency

For each milestone:

1. Identify the minimal relevant specification sections, existing modules, and tests.
2. Make the smallest coherent cross-module change that solves the problem, including necessary tests.
3. Run targeted tests during iteration; run full native regression at integration gates and the final candidate review.
4. If tests fail, inspect diagnostic sections and fix root causes; keep full logs privately when appropriate instead of repeatedly placing long logs into model context.
5. Record changed files, proof of test execution, known risks, and the next independent work item in `docs/PROGRESS.md`.
6. Continue to the next reachable milestone if permitted. Stop or mark a specific part blocked when additional external access or a genuine product decision is required.

Do not spawn parallel agents to edit overlapping core API and header files concurrently. Independent documentation review, test-design research or platform research may be parallelized, then reconciled in one implementation thread.

## 4. Platform validation

- **Linux:** native CMake/CTest, appropriate read-only libudev/sysfs probes, negative fixtures; retain the existing closed ioctl allowlist unless new read-only queries are individually documented and verified.
- **macOS:** use native IOKit/Disk Arbitration and actual test evidence on Apple Silicon; never presume Darwin host support from portable tests.
- **Windows:** native MSVC compile and execution only when a suitable runner or Windows computer is reachable; MinGW cross-build does **not** count as native-runtime acceptance.
- **Hardware:** no actual disk writes or destructive capability probes. Fixture-only NVMe/ATA/USB support must be labeled synthetic, not physical verification.
- **CI:** inspect the result for the exact commit; a checked-in `.github/workflows/ci.yml` file is not a green run.

## 5. Documentation and source manifest

The repository has `tools/check_documentation.py`, which checks a sorted, complete `docs/FILE_MANIFEST.txt` and file-level source conventions. After placing this kit into a checkout, ensure the new paths are included, then run:

```sh
python3 tools/check_documentation.py
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The actual project may require host-specific dependencies. These commands are not claims of execution. Use suitable separate build directories for sanitizers; do not force unsupported sanitizer configurations.

## 6. Session handoff and final release gate

At the end of a completed milestone or when context is becoming large, update `docs/PROGRESS.md` with the working tree SHA/status, decisions, files, executed commands with result and skip reason, next task and known blockers. A new thread should inspect that file plus the active spec section, rather than reconstructing the project history from chat messages.

At the end of v0.2.0 implementation, request a separate critical review for unresolved identity races, read-only query safety, logical vs physical target confusion, JSON/schema regressions, native API cleanup and error propagation. The review is not a substitute for running tests.

**Owner handoff:** Codex produces a tested candidate and accurate release notes; the maintainer reviews diffs and CI/hardware evidence and decides on versioning, commit, push, tag and publication. `IMPLEMENTATION_COMPLETE` is not proof of release readiness.

## 7. Project-specific historical constraints

Previous DiskSan work used a Mac source workspace and an isolated Linux VM, with selected-file transfer and hash verification. If Codex actually uses that path, it should discover the current directories, transfer only relevant files and compare source hashes before building. It must not assume paths belonging to esdumanOS, use `rsync --delete` on unrelated folders, or change system-wide package configuration unnecessarily.

Source references: <https://github.com/iamfurkann/DiskSan> and <https://developers.openai.com/cookbook/examples/codex/using_goals_in_codex>.
