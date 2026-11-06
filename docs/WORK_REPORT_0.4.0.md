# DiskSan v0.4.0 - engineering handoff

Status: IMPLEMENTATION_COMPLETE with platform limits. The CLI and optional Qt desktop passed native read-only tests on macOS and Debian 13. The Windows CLI passed cross-compilation and archive checks. Native Windows, hosted CI on the reviewed commit and maintainer release approval remain open.

## Source provenance

- Base branch/HEAD: `main`, `02beaae641228864af161a17563a01d62850ad6b`, the historical published 0.3.0 baseline.
- Work is uncommitted. No commit, push, tag, release, upload, deployment or repository-setting change was performed.
- Verification checkpoint: private `m6-verification-source`, 154 maintained files with individually checked SHA-256 hashes. Receipt `m6-verification-sha256.json` SHA-256: `a56305c22b22a259f81e7ccd4192f799ca7fa4c56b3e613e29abea980305c394`. Mac build-source equality was checked; all 154 hashes matched after VM transfer.
- Final desktop checkpoint: `m7-text-source`, receipt SHA-256 `f88f5aa407ce8676da5287674bdfa010b2f5df79061258821010e8b6ed6fcc77`. Seven inputs under `desktop/` changed after M6. Changes cover startup argument rejection, failed child startup, whitespace and plain ASCII decoding within the parser deadline, and the process/keyboard/GUI test harness. The other 99 build, source, test and tool inputs match M6, including every standalone CLI input. The filename/SHA-256 map for all 106 current inputs, serialized as sorted compact JSON, hashes to `aabc46f1cf06b3aa5c1da1d19ce04bbe0e5b73ad1881355e2edc360cce0ca230`. The separate delivery receipt covers the final documentation and archive bytes.
- Incoming user changes and older specifications/reports remain preserved. Eight frozen native protocol source/audit hashes still match the 0.3.0 reference.

## Files to review

| Area | Review entry points |
|---|---|
| C17 planner and output | [Public API](../include/disksan/plan.h), [policy](../src/core/plan.c), [serialization](../src/output/plan_output.c), [CLI dispatch](../src/main.c) |
| Portable and fault coverage | `tests/test_plan.c`, `tests/fixture_plan.c`, `tests/plan_allocator.c`, `tests/test_plan_cli.py` |
| Desktop alpha | `desktop/main.cpp`, three backend components, three QML files and seven fixture/test files. The [desktop file map](GUI_ARCHITECTURE_0.4.0.md#implemented-file-layout) lists them. |
| Build and CI | Root and desktop CMake, [manual desktop workflow](../.github/workflows/desktop.yml), Doxyfile and `tools/check_documentation.py` |
| Contracts and delivery | [Threat model](THREAT_MODEL_0.4.0.md), [simulation contract](SIMULATION_CONTRACT_0.4.0.md), [acceptance](ACCEPTANCE_0.4.0.md), [test matrix](TEST_MATRIX_0.4.0.md), [release checklist](RELEASE_CHECKLIST_0.4.0.md) and [complete maintained file list](FILE_MANIFEST.txt) |

## Requirement-to-implementation audit

| Specification area | Implemented evidence | Verification scope |
|---|---|---|
| 1-2: product and existing contracts | No destructive backend; unchanged native adapters/protocol allowlist; existing metadata-only target window | Source/hash audit, old CLI suites, 39 baseline fixture comparisons and explicit-probe synthetic control |
| 3: bounded portable plan | `plan.h`, `plan.c`: owned inline assessment copy, reconstructed findings, bounded enums/strings/counts, hazard precedence and independent prerequisites | 14,206 policy/output assertions; no model allocation or OS call |
| 4: additive CLI | `main.c` fixed plan/simulate grammar, opaque selector, before/after revalidation and old exit classes | 21 plan/simulation unittest cases including forbidden options, stale/denied/missing targets and output failure |
| 5: schema and transcript | `plan_output.c`: schema 2, unchanged device record, explicit provenance, false authority/verification and ten fixed modeled steps | Exact uint64/UTF-8, deterministic output, invalid-derived-model pre-write rejection and eight-allocation fault sweep per verb |
| 6: future threat design | `THREAT_MODEL_0.4.0.md`: independent observation, authorization, operation/recovery and verification responsibilities | Reviewed design-only boundaries; no endpoint, token, command dispatcher or service implementation |
| 7: working optional desktop | Real Qt Quick/QML screens, pinned known CLI, enum argv, strict schema/types, one asynchronous child and generation invalidation | Actual child processes and QML interaction on macOS and Linux, including native metadata and Cocoa keyboard coverage |
| 8: negative/fault tests | Forbidden-query adapter, malformed/overflow/permission/race/cancel fixtures; no automatic physical probe | CLI tripwire positive control; desktop fixed-argv/schema/process suites and actual error views |
| 9: build/docs/packages | Default OFF, explicit missing-Qt failure, C/C++ separation, English/ASCII briefs, updated public documents and source manifest | Forced-no-Qt CLI build, Doxygen, native dependency review and exact contents of all three archives |
| 10: M0-M7 evidence | Versioned progress, acceptance/test matrix, candidate notes and this handoff | M0-M7 local engineering evidence is recorded below. External publication gates remain open. |

Plans and simulations describe the selected observation. A fixed modeled step cannot submit, complete or verify an operation. Unknown scope remains unknown; `inspection_only` does not imply that a device could safely be erased.

## Executed tests

| Environment/checkpoint | Command family | Result and limitations |
|---|---|---|
| macOS 27.2 arm64, AppleClang 21, CMake 4.4.1, Qt 6.12.0, M7 desktop source | Release build; full CTest in `build-v040-desktop` | PASS 26/26 groups; native discovery/assessment and QML actually ran; unsupported-host CLI subcase SKIP |
| Same Mac/source, Debug ASan/UBSan | Full CTest in `build-v040-desktop-sanitize` | PASS 26/26; same CLI subcase SKIP; prebuilt Qt itself is not sanitizer-instrumented |
| Same Mac/source, Cocoa/software scene graph | `test_desktop_ui fixtureJourney keyboardAndAccessibility selectedFailure nativeInventory` | PASS 6 QtTest entries including init/cleanup, no skips; actual keyboard/arrow navigation, text selection, explicit contrast palettes and live read-only metadata |
| Mac Qt-disabled configuration | OFF plus disabled Qt discovery; build, portable CTest and packaging | PASS 17 portable groups plus package smoke; CLI links only native system libraries; explicit ON with disabled Qt fails clearly |
| Debian 13 x86-64, M3 intermediate snapshot | GCC Release and Clang ASan/UBSan full CTest | PASS 26/26 each; unsupported-host CLI subcase SKIP; not a substitute for final M6 source |
| Linux-hosted MinGW, M3 intermediate snapshot | All-target Windows cross-build | PASS compilation only; no native Windows execution |
| Debian 13 x86-64, final M6 executable source | GCC Release and Clang ASan/UBSan full CTest | PASS 26/26 each; unsupported-host CLI subcase SKIP; native assessment and Linux discovery actually ran |
| Linux-hosted MinGW, final M6 executable source | All-target Windows cross-build | PASS with MinGW-w64 GCC 14.0; native Windows execution NOT_RUN |
| Debian 13 x86-64, Qt 6.12.0, M7 desktop source | GCC Release and Clang Debug ASan/UBSan, five desktop CTest groups per mode, offscreen/software renderer | PASS 5/5 in each. The DiskSan code was instrumented in the sanitizer build. The vendor Qt SDK was prebuilt. Parser limits, CLI contracts, process races, fixture keyboard journeys and native read-only inventory all ran. No QtTest subcase skipped. The production app also opened and waited for input with no active child at the observation point. |
| Native Windows/MSVC 0.4.0 and Windows Qt | No accessible native runner used | NOT_RUN |
| Doxygen C/C++/Python/Markdown | Doxygen 1.18.0 on macOS, warnings as errors | PASS on the 156-file handoff tree, HTML index generated |
| All three CLI archives | CPack, exact contents and binary hashes, native package smoke | PASS for 47 files per archive. macOS and Linux exercised the relocated CLI through version, help and all six inspection commands. Windows ZIP integrity, PE architecture, imports and contents passed. Native Windows execution is NOT_RUN. |
| Optional CI workflow | PyYAML parse; read-only permissions | PASS local parse of both final YAML files, Linux ICU selection and manual desktop trigger; hosted runs NOT_RUN |

The desktop contract suite now includes eight Python cases. The real child-process suite passed 11 QtTest entries including setup/cleanup. The Cocoa keyboard journey also passed two separate repetitions with the synthetic window active for key delivery.

Exact commands and private log basenames appear in [progress](PROGRESS_0.4.0.md). Tests write ordinary disposable fixtures only. No physical capability query, storage user-data access, mount modification, elevation or destructive test was performed.

## Desktop and deployment scope

The app starts disconnected and performs discovery only after Refresh. Its fixture executable is separate and permanently labeled DEMO DATA. A native app cannot accept an arbitrary executable path or command from QML. Identity comparisons and response freshness end with the observation; the GUI retains no future authority.

Parser limits are 2 MiB stdout, 16 KiB decoded field, 32,768 nodes, depth 16 and 1,024 collection entries; inventory is capped at 256 records. Stderr is capped at 32 KiB and never echoed. Each child has a 15-second deadline; JSON parsing uses a cooperative 500 ms deadline. These are application resource limits, not hardware safety guarantees.

The GUI is source-build alpha only. There is no relocatable GUI asset or install rule in CLI packages. Qt 6.12's desktop macOS target is 14.4; actual testing used 27.2, not the minimum or Intel hardware. Windows Qt, other Qt minors/architectures, native Linux window-system coverage, end-to-end VoiceOver/NVDA/AT-SPI and automatic OS-theme changes remain unverified on this candidate. See [desktop license/deployment review](GUI_ARCHITECTURE_0.4.0.md).

## Candidate and maintainer gates

The three platform-specific CLI archives and `SHA256SUMS` are under `dist/v0.4.0/`. Checksums in that file cover archive bytes. The following digests cover the executables inside the archives.

| Target | Executable SHA-256 | Runtime inspection |
|---|---|---|
| macOS arm64 | `1005fed111022f88dd4049aebf8634fbe65904f0d78582c9e2a275584a0b6a83` | Mach-O arm64, macOS deployment target 12.0, three native frameworks and libSystem |
| Linux x86-64 | `e0f11357ffc96a2ad719592760966d4fdedf7817ec3b88cf8f835d57716e1020` | ELF64, libudev.so.1 and libc.so.6, maximum referenced GLIBC version 2.34, no RPATH/RUNPATH |
| Windows x86-64 | `9d0d41204c6b4239e8745003551208213a126c43c6341a62fbbafa7c6fcd21da` | PE32+ console executable, KERNEL32.dll, msvcrt.dll and SETUPAPI.dll |

Before publication, the maintainer must review and commit the final source, observe CLI CI on that exact commit including native MSVC, decide whether to run the optional desktop workflow, inspect individual skips and accept the documented hardware/GUI limits. The owner's Windows runtime acceptance remains separate from cross-compilation and hosted synthetic tests. Tagging, upload and publication require explicit maintainer action. No signing/notarization or safe-erasure certification is claimed.
