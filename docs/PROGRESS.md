# DiskSan 0.2.0 progress and handoff

> Execution journal, not a declaration of success. Update after each completed, evidenced milestone or before a context/session handoff. Avoid copying raw logs or sensitive device inventories into the repository.

## Current state

- Target version: `0.2.0` (source implementation verified; source declares 0.2.0).
- Baseline examined: `main` HEAD `58d07c9a2f00f9a21c4e6c6a7ac96d9febe332bf` on 2026-10-26; revalidate against actual working tree.
- Active milestone: `M7 complete - Maintainer review and external release gates`.
- Status: `IMPLEMENTATION_COMPLETE` for the accessible acceptance scope; publication gates remain explicit.
- Source-of-truth specification: `docs/specs/v0.2.0.md`.
- Public v0.1.1 tag/release: `NOT_CONFIRMED` as of research snapshot; check again if it affects task.

## Milestone tracker

| ID | Outcome | Status | Evidence / comments |
|---|---|---|---|
| M0 | Inspect current baseline, tests, constraints; freeze 0.2.0 contracts | PASS | macOS baseline reproduced; two incoming-document/package gate failures repaired; design in ASSESSMENT_DESIGN.md |
| M1 | Portable policy and reason/evidence model | PASS | Initial 1,263 assertions; expanded to 1,291; Mac Release and ASan/UBSan; Linux M1 GCC/Clang full 12/12 |
| M2 | Read-only target resolution and freshness contract | PASS | 617 portable assertions; native Linux/macOS selected-window and alias checks |
| M3 | Topology/usage evidence and unknown-safe evaluation | PASS | Linux/macOS implemented and tested; Windows adapter cross-build PASS, native runtime NOT_RUN |
| M4 | Evidence-based capability assessment | PASS | Six methods, four independent unknown dimensions; no protocol probes; synthetic state/conflict coverage |
| M5 | `assess` CLI, JSON/text contracts, compatibility | PASS | 11 new CLI tests, 18 preserved inventory-output tests; both commands retain snapshot limits |
| M6 | Cross-platform fixtures, faults, sanitizers, integration | PASS | Final Mac 15/15 Release and ASan/UBSan; Linux 18/18 GCC and Clang ASan/UBSan; Windows cross-build only |
| M7 | CI, docs, packaging, release readiness handoff | PASS | Requirement/source audit, docs and three platform archive checks PASS; Windows archive is inspection-only, runtime NOT_RUN |

Status vocabulary: `NOT_STARTED`, `IN_PROGRESS`, `PASS`, `FAIL`, `BLOCKED`, `NOT_RUN`, `SKIP`.

## Baseline milestone evidence

- ID: M0, 2026-10-26.
- Working-tree reference: `58d07c9a2f00f9a21c4e6c6a7ac96d9febe332bf` plus the maintainer's instruction kit and the repairs below. No commits or remote mutations performed.
- Incoming changes preserved: AGENTS.md, baseline/goal/workflow/progress/spec documents and manifest. Private pre-edit copies retained under `.local-evidence/v0.2.0/user-input/`.
- Build: `cmake -S . -B build-v020-baseline -DCMAKE_BUILD_TYPE=Release`; `cmake --build build-v020-baseline --config Release`: PASS, native AppleClang, warnings as errors.
- Initial `ctest --test-dir build-v020-baseline -C Release --output-on-failure`: core, identity_faults, windows_metadata, output_contract and cli PASS. Documentation FAIL on Unicode tree lines in the incoming workflow guide; packaging FAIL because the checker missed nested docs/specs; macos_integration failed because sandbox blocked diskutil.
- Repairs: replace only the guide's tree glyphs with ASCII; make package expectations recursive to match CMake's recursive documentation installation.
- `ctest --test-dir build-v020-baseline -C Release -R '^(documentation|packaging_smoke)$' --output-on-failure`: PASS, 2/2.
- `ctest --test-dir build-v020-baseline -C Release -R '^macos_integration$' --output-on-failure`, outside sandbox with read-only access: PASS, 1/1. All eight baseline groups now have passing evidence; CLI retains its expected unsupported-host SKIP.
- Linux baseline at the same HEAD subsequently reproduced: PASS, 11/11 groups with GCC Release in the isolated VM. Native Windows baseline in this goal: NOT_RUN. Historical results are not reused as current execution evidence.
- Decisions: preserve schema-2 inventory, use a distinct operation envelope, no protocol commands initially, separate completeness from negative hazard observations. See `ASSESSMENT_DESIGN.md`.

## Current blockers

- Windows native validation and physical hardware matrix were open in the 0.1.1 report; check actual access and current evidence.
- Any unavailable hardware/API must remain `unknown` or `not_verified`, never be inferred as a passing test.

## Next action

The maintainer owns native Windows acceptance, hosted CI on a committed revision,
review and publication. Keep Windows preview status explicit and use the manual
acceptance steps in RELEASE_NOTES_0.2.0.md. No independent implementation milestone
remains open after the final source audit; checksums accompany the local archives.

## Compact milestone entry template

### M1 - Portable evidence and policy | PASS | 2026-10-26

- Added `include/disksan/assessment.h`, `src/core/assessment.c`, `tests/test_assessment.c` and CMake/manifest registration. No native API or device-access operation added.
- Owned fixed-size observations distinguish unknown, false and true; method support, access, applicability and passthrough remain independent. Policy preserves conflicting sources, keeps observed hazards blocking, treats missing/partial evidence as review, and always includes a non-authorization finding.
- Invalid enum/string or capacity exhaustion poisons evaluation with `not_evaluated`; exact duplicates do not exhaust storage. No new heap allocation paths exist in this milestone.
- Fixtures cover disk/namespace/partition/APFS volume/snapshot, loop/DM/MD/multipath/composite roles, mount/root/swap/read-only/holder/backing hazards, unknown dimensions, denied/unavailable capability evidence, contradictions, stable ordering, copied-buffer ownership, repeated reset and exact bounds.
- `build-v020/test_assessment`: PASS, 1,263 assertions.
- `cmake -S . -B build-v020 -DCMAKE_BUILD_TYPE=Release`, build and `ctest --test-dir build-v020 -C Release -L portable --output-on-failure`: PASS, 7/7 groups, AppleClang 21.0.0.
- Equivalent Debug build in `build-v020-sanitize` with `-DDISKSAN_SANITIZERS=ON`, portable CTest: PASS, 7/7; ASan/UBSan enabled. Existing inventory/CLI tests remain passing.
- `python3 tools/check_documentation.py`: PASS, 63 maintained files, 178 C declarations/definitions, 57 Python functions/classes. `git diff --check`: PASS.
- Native assessment collectors, resolver window, CLI/rendering and capability provider remain unimplemented. This milestone does not claim live topology, protocol support, native Windows or hosted CI validation.
- Next independent work: portable target-window comparison and race/error fixtures; current Linux baseline and M1 compiler checks in the isolated VM.

### M2/M3 portable seams and metadata provider | PASS (partial milestone) | 2026-10-26

- Added target comparison/window APIs, metadata-only capability provider, and portable Linux proc parsers. Native usage collection is declared but not implemented yet; it is substituted by a synthetic adapter only in `test_target`. The production CLI still exposes only existing commands and links no new target execution path.
- `target_window`: 617 assertions covering changed generation/devnum/role/size/parent/controller/read-only/holder state, identity evidence loss, unknown generation, Windows interface limitations, duplicate/missing selection, two-stage discovery errors, usage failures, exhausted evidence, original alias-selector reuse and cleanup. Actual native symlink/hotplug acceptance for the new window remains NOT_RUN.
- `assessment_policy`: expanded to 1,291 assertions, including every true/false/unknown subset for mounted hazards. Known hazards cannot be downgraded by unknown/contradictory evidence.
- `usage_parsers`: 622 assertions covering escaped paths, repeated mount points, missing fields, malformed/overflow numbers, signed swap priorities, embedded NUL/control bytes, oversized lines/paths, all byte values, truncated records and empty output on parse failure. Host-wide scope and file backing are explicitly not inferred by parsing.
- macOS Release and ASan/UBSan portable CTest before the parser addition: PASS, 8/8 groups. The parser and documentation then passed targeted Release checks; parser also passed ASan/UBSan after reconfiguring CMake for the new target. An initial sanitizer target invocation failed because the old build configuration did not yet contain that target; reconfiguration resolved it.
- `python3 tools/check_documentation.py`: PASS, 71 maintained files, 216 C declarations/definitions, 57 Python classes/functions. `git diff --check`: PASS.
- M1 VM evidence is from an immutable earlier 63-file snapshot, not the later target/parser sources: GCC Release full 12/12, Clang ASan/UBSan full 12/12; MinGW all-target cross-build PASS, runtime NOT_RUN. Baseline HEAD archive SHA256 and all source MD5s matched after transfer.
- Current 71-file portable snapshot was independently copied to the VM with all 71 MD5s matching. GCC Release and Clang ASan/UBSan targeted CTest (`documentation`, `assessment_policy`, `target_window`, `usage_parsers`): PASS, 4/4 each. MinGW cross-build of all three new test executables: PASS; native Windows runtime remains NOT_RUN. Cross-compilation is not native-runtime evidence.
- Read-only review: `READ_ONLY_AUDIT_0.2.0.md` covers the implemented pure modules and existing discovery calls. No new OS queries or protocol command exists yet. Final native-boundary trace remains NOT_RUN.
- Next: Linux usage graph and native proc/stat metadata collection, followed by macOS/Windows adapters and CLI/rendering. This is not IMPLEMENTATION_COMPLETE or READY_TO_PUBLISH.
- Latest local package gate: `ctest --test-dir build-v020 -C Release -R '^packaging_smoke$' --output-on-failure`: PASS. This is still a 0.1.1-versioned development archive, not a 0.2.0 candidate artifact.

### M2-M6 integrated native assessment | PASS (verified scope) | 2026-10-26

- Added bounded observed graph traversal, Linux mount/swap and loop-backing metadata, macOS Disk Arbitration/cached mount metadata, and Windows zero-access volume-extents/mount-path queries. All retain explicit incomplete topology/usage/physical scope; no protocol capability command exists.
- Added `assess` and `capabilities`, both using the selected before/after identity window. Fatal changes return no success payload; blocked/review observations still return inspection exit 0. JSON remains schema 2 with the unchanged inventory device object and literal `erase_authorized: false`. Source version is now 0.2.0.
- macOS Release and ASan/UBSan full CTest: PASS, 15/15 groups each, including live target/alias/root checks, 474 synthetic native-usage ownership/fault assertions, 181 graph assertions, 11 assessment CLI tests and the existing 18 output tests. Existing CLI unsupported-host case: SKIP on native host, expected.
- Initial native Mac invocation failed because sandbox denied Disk Arbitration/diskutil; repeating the read-only test outside that sandbox exposed a test field-name typo (`access` versus `os_access`). The fixture was corrected, and native Release/sanitizer tests passed. No product fallback was weakened.
- Linux source snapshot `m3-integrated-source`: all 84 maintained-file SHA-256 hashes matched after transfer to the dedicated DiskSan VM directory. GCC Release full CTest: PASS, 18/18; native root, aliases, by-id and usage integration passed; 20 Linux usage fault scenarios include permission failures, special-files/non-proc substitution, malformed/oversized input, loop backing/file changes and descriptor cleanup. Assertion count 26,042 includes per-byte fixture construction, not that many independent scenarios.
- Windows MinGW all-target cross-build from that same 84-file snapshot: PASS. New volume response buffers are bounded stack storage; UTF-16/MULTI_SZ fixtures passed on Mac/Linux. Native Windows execution/MSVC: NOT_RUN, not implied by cross-compilation.
- Mac and Linux package smoke tests passed with 0.2.0 archive names. These are development test packages, not final release assets; packaged documentation is still changing. Linux Clang sanitizer run is in progress.
- Subsequent final-review edits (not covered by those earlier Linux logs): nonblocking metadata opens prevent FIFO-open stalls, closing-snapshot inventory bounds precede loop probes, text output retains method-level provenance. Reverification required. macOS native fault files were added after the 84-file VM snapshot.
- Private logs: `.local-evidence/v0.2.0/m3-macos-release-tests.log`, `m3-macos-sanitize-tests.log`; VM `m3-integrated-gcc-tests.log` and `m3-integrated-windows-build.log`. Raw inventories stay private. No commits, tags, push, CI dispatch or publication performed.

### M6 final regression and requirement audit | PASS | 2026-10-27

- Final behavior snapshot `m6-final-source`: 89 maintained files transferred with exact SHA-256 equality. Subsequent Mac/Linux/core changes are documentation/manifest only. The later Windows system-directory API correction is separately cross-built below; all other code hashes match.
- Mac AppleClang Release: `cmake --build build-v020 --parallel 2` and `ctest --test-dir build-v020 -C Release --output-on-failure -V`: PASS 15/15. Equivalent Debug ASan/UBSan in `build-v020-sanitize`: PASS 15/15. Logs: `.local-evidence/v0.2.0/m6-macos-{release,sanitize}-tests.log`.
- Linux GCC Release in `m6-final-gcc`: PASS 18/18; Clang Debug with `-DDISKSAN_SANITIZERS=ON` in `m6-final-clang`: PASS 18/18. Both include native root/alias integration and new nonblocking metadata, loop-link retention allocation failure, stream I/O failure and cleanup checks. Linux usage now has 22 scenarios and 26,147 assertions (many count fixture construction bytes). Logs copied to `.local-evidence/v0.2.0/vm-logs/m6-final-{gcc,clang}-tests.log`.
- Windows MinGW all-target build in `m6-final-windows`: PASS, including 16-scenario `test_windows_usage.exe`. This fixture asserts exact zero-access flags and the single metadata IOCTL, malformed counts/ranges/UTF-16/MULTI_SZ, lost/changed closing query and handle closure. Native execution and MSVC: NOT_RUN. Log: `vm-logs/m6-final-windows-build.log`.
- Final source review traced new calls through CLI, target window, graph, usage and capability modules; no destructive/data-access/backend or elevation path was introduced. Exact queries, limits and evidence gaps are in READ_ONLY_AUDIT_0.2.0.md. Full native syscall tracing is not claimed.
- README, architecture, schema, testing, roadmap, coding/security/distribution docs, release notes and requirement matrix updated. `python3 tools/check_documentation.py`: PASS, 90 files, 341 C declarations/definitions, 75 Python functions/classes. `git diff --check`: PASS.
- CMake package smoke gates passed for final source behavior on Mac/Linux. Delivery packaging stages the exact tested Mac/Linux or cross-built Windows executable with the current public documents, then uses CPack and the same exact-content/relocated-smoke checker. An initial private staging config omitted CPack's description; adding it fixed the packaging error. The Mac/Linux archive content and relocated smoke checks passed; the Windows content/CRC check passed with `--no-run`. Final per-platform receipts/checksums are stored outside maintained docs to avoid self-referential archive hashes.
- [ACCEPTANCE_0.2.0.md](ACCEPTANCE_0.2.0.md) maps every specification section and numbered validation group to source/tests or an explicit external NOT_RUN gate. No critical local safety/identity failure remains known.

### M7 delivery review | PASS with external gates | 2026-10-27

- Final requirement audit: ACCEPTANCE_0.2.0.md. Maintained Markdown local links, ASCII/manifest/brief checker, and `git diff --check`: PASS. All new native query names, access modes, bounds, cleanup and limitations are documented.
- API reference review found that GetWindowsDirectoryW can return a per-user Terminal Services directory. Replaced it with GetSystemWindowsDirectoryW and updated its synthetic substitution/provenance. Exactly two Windows-specific code files changed after M6; all 90 files in `m8-windows-source` matched transferred SHA-256 values. Production `disksan` and `test_windows_usage` then cross-built successfully with MinGW warnings as errors. Native runtime remains NOT_RUN; no Mac/Linux/core code changed.
- Three CPack delivery archive checks PASS: exact executable bytes and public-document allowlist, ZIP CRC/TGZ file types, relocated native version/help on Mac/Linux; Windows uses explicit `--no-run`. Final journal edits require the usual document refresh and archive regeneration; external receipt JSON and SHA256SUMS identify the delivered bytes without embedding self-referential hashes here.
- Dependency review: Mac links only existing system frameworks/libSystem; Linux needs libudev.so.1 and libc.so.6, maximum recorded GLIBC requirement 2.34, no RPATH/RUNPATH; Windows imports KERNEL32.dll, msvcrt.dll and SETUPAPI.dll. This is static dependency evidence, not older-OS runtime validation.
- Local outputs are under ignored `dist/v0.2.0/`; private compile/test/package evidence remains under `.local-evidence/v0.2.0/`. No device inventories, credentials, build sources or local logs are included in runtime archives.

## Final handoff

- Implemented: portable observations/policy, bounded target/graph revalidation, Linux/macOS/Windows metadata usage adapters, independent unknown method evidence, `assess`/`capabilities` text/JSON, faults and read-only integration tests.
- Compatibility: `list`, `info`, schema-2 device fields, unknown/null semantics, selector behavior and exit classes preserved. New operation envelope is documented; blocked assessment exits 0 because inspection completed, always with false authorization.
- Evidence: Mac Release and ASan/UBSan 15/15; Linux GCC Release and Clang ASan/UBSan 18/18; Windows MinGW compilation PASS, native runtime/MSVC NOT_RUN. Existing unsupported-host CLI SKIP expected; no available native assessment prerequisite skipped.
- Audit: read-only source boundary PASS; no executable destructive command, device-data read/write or privilege escalation. Snapshots remain non-atomic and expire at observation end.
- External gates: native Windows including prepared fault execution BLOCKED by platform access/NOT_RUN; hosted CI for this uncommitted change NOT_RUN; real controller/bridge/hotplug/stack matrix NOT_RUN. Owner steps are in ACCEPTANCE_0.2.0.md and RELEASE_NOTES_0.2.0.md.
- Publication: no staging, commit, push, tag, repository setting or release action performed. Future title `DiskSan v0.2.0 - Read-only Target Assessment`, future tag `v0.2.0`; both require maintainer action.
- Readiness: WITH_LIMITATIONS for source/candidate review; not READY_TO_PUBLISH. Windows stays preview. Final archive checksums are external assets and do not imply native Windows validation.
