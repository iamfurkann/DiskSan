# DiskSan v0.3.0 progress and verification ledger

Status: IMPLEMENTATION_COMPLETE, WITH_LIMITATIONS. M0-M7 pass within the candidate scope documented below. The audited Linux provider is enabled only for explicit probing; other platforms compile the unavailable adapter. Physical protocol acceptance, current Windows/MSVC execution and final committed-source hosted CI remain NOT_RUN. This is an uncommitted candidate handoff, not release authorization or publication.

## Actual working checkout (Codex must fill before modifying code)

- Inspected date: 2026-10-27.
- Branch and HEAD: `main`, `d00be042e9a64552b86b3fc807d2995e4110b165`; local `v0.2.0^{}` resolves to that same commit.
- Dirty files preserved: modified AGENTS.md, CODEX_GOAL.md, CODEX_WORKFLOW.md and FILE_MANIFEST.txt; eight untracked 0.3.0 instruction documents; missing historical v0.2.0 specification. Initial status/diff and copies of every present input are private under `.local-evidence/v0.3.0/user-input/`. No source changes were present initially. Restored the missing historical specification exactly from the tag, as required by AGENTS.md and the incoming manifest/workflow guide.
- Published reference tag: `v0.2.0` -> `d00be042e9a64552b86b3fc807d2995e4110b165` (historical public checkpoint).
- Baseline CMake/CTest and warnings: AppleClang Release, warnings as errors, full 15/15 PASS after the document repair and read-only sandbox access correction described below.
- Available native systems and tools: macOS arm64 AppleClang 21.0.0; reachable dedicated Debian VM, kernel `6.12.101+deb13-amd64`, Linux NVMe UAPI header present. Native Windows/MSVC not locally available. The existing project-local VM dependencies were located without changing host configuration.
- Exact existing regression test group counts: current Mac baseline 15 groups (11 portable); historical tagged CI separately verified below. New M1 adds one portable group.
- Source-of-truth specification: `docs/specs/v0.3.0.md`.

Status terms: `NOT_STARTED`, `IN_PROGRESS`, `PASS`, `FAIL`, `BLOCKED`, `NOT_RUN`, `SKIP`.

## Milestone plan

| ID | Work item | State | Evidence / blocker |
| --- | --- | --- | --- |
| M0 | Inspect baseline, freeze compatibility, audit current native query APIs | PASS | Actual checkout, published tag/run, Mac baseline, pinned kernel/API and NVMe field review |
| M1 | Portable decode/normalization and query evidence model | PASS | 12,157 decoder/evidence assertions; Mac Release/sanitizer portable 12/12; Linux GCC full regression and Clang portable; MinGW all-target build |
| M2 | Closed query allowlist, threat model, forbidden-opcode/flag tests | PASS | Actual ioctl bytes captured with GCC and Clang ASan/UBSan; descriptor/mapping lifecycle remains an M4 gate |
| M3 | Opt-in `capabilities --probe`, CLI and stable JSON reporting | PASS | Mac Release/sanitizer full 18/18, Linux GCC/Clang full 22/22; 38 baseline comparisons; MinGW all-target build |
| M4 | Linux fixed NVMe Identify Controller provider | PASS | GCC/Clang synthetic boundary 4/4; integrated full suites 24/24; no physical query |
| M5 | Conditional Windows property provider; explicit unavailable alternatives | PASS within available scope | Actual unavailable adapter: 888 assertions; Windows property provider not enabled without MSVC/API acceptance |
| M6 | Native seams, fuzz/negative fixtures, sanitizer/regression tests, privacy review | PASS available native scope | Final Mac full 19/19 each, Linux full 24/24 each; Doxygen repaired and passed; 16 archive fault cases |
| M7 | Hosted CI, hardware acceptance handoff, package/docs and release readiness | PASS candidate handoff scope | Three checked candidate archives; release notes, acceptance and manual gates supplied; current CI/MSVC/physical probes NOT_RUN |

## Verified milestone template

### M0 - Actual baseline and initial query audit | PASS | 2026-10-27

- Source: `d00be042e9a64552b86b3fc807d2995e4110b165` plus the preserved incoming instruction kit; no executable source edits during baseline testing.
- GitHub GET evidence confirms the [v0.2.0 prerelease](https://github.com/iamfurkann/DiskSan/releases/tag/v0.2.0), three platform assets and SHA256SUMS. [Run 40546829221](https://github.com/iamfurkann/DiskSan/actions/runs/40546829221) completed successfully on exactly the local/tagged SHA: Linux GCC, Linux Clang ASan/UBSan, macOS AppleClang and Windows MSVC jobs all passed. This is historical 0.2.0 evidence, not 0.3.0 CI or physical-controller validation. Downloaded archive/source equivalence was not tested.
- `cmake -S . -B build-v030-baseline -DCMAKE_BUILD_TYPE=Release` and `cmake --build build-v030-baseline --parallel 2`: PASS with AppleClang 21.0.0 and warnings as errors.
- Initial CTest: FAIL in documentation (missing historical specification) and two native Mac groups (sandbox denied Disk Arbitration/diskutil). Restored only the missing historical document from the tag; reran the existing read-only integration tests outside the sandbox, without sudo or protocol queries: 2/2 PASS. Final full `ctest --test-dir build-v030-baseline -C Release --output-on-failure`: 15/15 PASS, including packaging. Private logs: `m0-macos-{configure,build,tests,native-tests,final-tests}.log` in the evidence directory. Expected unsupported-host CLI case remains SKIP.
- Documentation checker: PASS, 98 maintained files before M1. `git diff --check`: PASS. Original 0.2.0 progress, acceptance and release notes remain unchanged.
- Research: Linux v6.12 `nvme_cmd_allowed` allows the specific controller Identify CNS without requiring a write descriptor; the surrounding ioctl can execute dangerous commands and is not itself a safety boundary. The implementation will accept one fixed constructor, one controller query, no retries or alternate nodes, and an independently verified namespace/controller mapping. Denial stays unavailable; no elevated fallback. LN-01 is not ACTIVE until native constructor/mapping/fault verification completes.
- Reviewed NVMe 1.3d, 1.4c and 2.0c SANICAP layouts, reserved bits, controller type and version rules. Support bits will not imply method access, applicability or passthrough. Windows uses a different property-query contract and remains conditional on its own native validation.
- Physical protocol probes, new Windows MSVC fixtures and final 0.3.0 CI: NOT_RUN. Next: verify the pure decoder/evidence milestone, then implement and capture the exact Linux request constructor before enabling an opt-in path.

### M1 - Pure decoder and independent capability evidence | PASS | 2026-10-27

- Added `include/disksan/protocol.h`, `src/core/protocol.c` and `tests/test_protocol.c`, with CMake/manifest registration. No OS API, new handle, CLI option or production protocol invocation was added. Source version remains 0.2.0 until CLI integration.
- Exact-size, unaligned-safe byte decoding; explicit 1.3.0/1.4.0/2.0.0 guards; documented endian offsets, SANICAP reserved/consistency checks, invalid vendor/type/namespace limits, and cleared numeric output on every failure. Only support for three NVMe methods is derived; all other 21 dimensions remain unknown. Conflicting/mixed provider insertion is rejected, never silently truncated.
- Decoder/evidence suite: PASS, 12,157 assertions, including every short length 0..4095, oversized/SIZE_MAX/null input, all eight support combinations for each reviewed version, unknown versions, reserved-bit mutations, controller types, independent dimensions, rejected provider mixing and 1,024 deterministic arbitrary-buffer trials. No raw controller strings are retained.
- `cmake -S . -B build-v030 -DCMAKE_BUILD_TYPE=Release`, build, and `ctest --test-dir build-v030 -C Release -L portable --output-on-failure`: PASS 12/12. Debug `build-v030-sanitize` with `-DDISKSAN_SANITIZERS=ON`: PASS 12/12. A test-only padding comparison was replaced by field comparisons for portability; the final decoder fixture was rebuilt and passed in both configurations. Logs are private under `.local-evidence/v0.3.0/m1-macos-*`.
- Immutable `m1-portable-source`: all 101 maintained-file SHA-256 values matched after transfer to the dedicated 0.3.0 VM directory. Linux GCC Release full CTest: PASS 19/19. Clang Debug ASan/UBSan portable CTest: PASS 12/12. MinGW Release all-target build: PASS, including `test_protocol.exe`; Windows execution NOT_RUN. Private logs: `.local-evidence/v0.3.0/vm-logs/m1-*`. No protocol query is part of these integration tests.
- Documentation/manifest/ASCII/brief check: PASS, 101 files and 358 C declarations/definitions at M1. `git diff --check`: PASS. No historical acceptance report was rewritten. Next: test the precise Linux request bytes and error propagation, then integrate the opt-in window with unavailable platform fallbacks.

### M2 - Fixed Linux query boundary | PASS | 2026-10-28

- Added `src/platform/linux/protocol_query.{c,h}` and `tests/test_linux_protocol_query.c`. The fixed primitive is compiled only into its synthetic test at this milestone. It accepts no opcode, CNS, length, timeout, metadata or native control dwords from users/callers; only the closed request kind and a caller-owned descriptor/buffer.
- Linker-wrapped ioctl captures the actual 72-byte request, exact `NVME_IOCTL_ADMIN_CMD`, opcode 6, CNS 1, NSID/unused fields zero, 4096 initialized reply bytes and 5000 ms request timeout. It explicitly rejects other ioctl families and arbitrary enum values, verifies no retries, and preserves EACCES/EPERM/EIO/ENOTTY/ETIMEDOUT/EINTR/ENODEV and positive NVMe failure status. All descriptors are synthetic; no native device is opened.
- `m2-query-final-source`: all 104 maintained-file hashes matched after transfer. GCC Release and Clang Debug ASan/UBSan each built `test_protocol` and `test_linux_protocol_query` with warnings as errors; `ctest -R '^(documentation|protocol_decoder|linux_protocol_query)$' --output-on-failure -V`: PASS 3/3 each. Logs: `.local-evidence/v0.3.0/vm-logs/m2-final-*`.
- The earlier M2 snapshot also passed; final review corrected the test wrapper's `va_arg` type to exactly match the supplied non-const pointer and reran both toolchains. A private snapshot helper initially resolved its root one level too high, failed before creating a snapshot, and was repaired before the final transfer. No unrelated files were changed.
- LN-01 remains CANDIDATE until M4 proves read-only open flags, controller/namespace association, revalidation and cleanup. The ioctl supplies no returned byte count; the fixed buffer/request contract and completion status are distinct from independent proof of truthful firmware contents. Physical probes and Windows native execution: NOT_RUN.
- Next: add opt-in-only reporting and target-window dispatch with synthetic probe fixtures, preserving default metadata behavior and literal false authorization.

### M3 - Explicit CLI, receipt and selected-target window | PASS | 2026-10-28

- Bumped candidate source version to 0.3.0. Added explicit-only `capabilities --probe`, a bounded additive receipt and one-provider selection inside the existing before/after target window. All builds initially use the no-query unavailable adapter. No real controller was probed.
- Eight Python probe CLI tests and 169 target/receipt assertions passed: option order, literal selectors, rejected duplicates/raw command options, no environment opt-in, no discovery on invalid grammar, zero probe calls from old fixtures, independent dimensions, changed targets, unavailable/denied/malformed replies, cleanup and output failure. Invalid internal receipts fail before output. `docs/DEVICE_MODEL.md` documents the additive fields and reasons.
- Compared 38 default list/info/assess/capabilities invocations against the actual 0.2.0 baseline fixture executable. Exit status, stdout and stderr matched byte-for-byte after replacing only the version string. Private receipt: `m3-default-compatibility.json`.
- macOS AppleClang Release and Debug ASan/UBSan portable suites: PASS 14/14 each. Full suites including existing live metadata integration and packaging: PASS 18/18 each. First sandboxed full run failed only where Disk Arbitration/diskutil were inaccessible; the same read-only suites passed outside that sandbox, without sudo or protocol hardware queries. Logs: `.local-evidence/v0.3.0/m3-macos-{final,sanitize-full}-tests.log`.
- Immutable `m3-opt-in-source`: all 112 maintained-file SHA-256 hashes matched in the dedicated VM directory. Linux GCC Release and Clang Debug ASan/UBSan full CTest: PASS 22/22 each, including synthetic fixed-request tests and existing metadata integration. MinGW all-target build: PASS, including the new probe fixtures. Windows native execution and final 0.3.0 hosted CI remain NOT_RUN. Private logs: `vm-logs/m3-*`.
- Documentation/manifest/ASCII/brief check: PASS at M3 with 112 maintained files and 394 C declarations/definitions. Whitespace check: PASS. Existing published-version progress/acceptance/release files remain unchanged.
- Next: verify Linux provider mapping/open/query/close fault fixtures, activate only the audited implementation, then verify actual production builds and unavailable alternatives.

### M4 - Linux production provider | PASS within tested scope | 2026-10-28

- Added `protocol_mapping.{c,h}` and the Linux `protocol.c` lifecycle around the fixed constructor. Fresh udev objects verify the selected block record, direct PCI NVMe parent, live transport, diskseq/NSID/CNTLID and node identities. The controller node comes from udev, never a guessed namespace suffix. Exactly one read-only, close-on-exec, no-follow, nonblocking open and one fixed Identify query are possible.
- Synthetic boundary source `m4-provider-reviewed-source`: all 116 hashes matched after transfer; GCC Release and Clang Debug ASan/UBSan targeted documentation/decoder/constructor/provider tests PASS 4/4 each. Native lifecycle fixture: 421 cases (255 are forbidden enum values), 279,111 assertions including per-byte initialized-buffer checks. It verifies fresh mappings, borrowed/owned udev cleanup, zero native calls before gating, complete open flags, changed nodes/generation/parent/NSID/CNTLID, permission/status/OOM failures, close errors including EINTR without retry, and actual normalized capability dimensions.
- The first reviewed Clang configuration was interrupted before building. Its local session handle and matching remote processes were absent; the same immutable source configuration was resumed, built and passed. No active run was restarted merely because observation timed out. Initial fixture snapshot also passed GCC before additional resource/evidence checks.
- After the audit was recorded, enabled exactly LN-01 in the Linux production target. Immutable `m4-m5-integrated-source`: all 117 maintained-file hashes matched. Production builds and full CTest PASS 24/24 in GCC Release and Clang Debug ASan/UBSan, including existing native metadata integration and package smoke. Logs: `vm-logs/m4-reviewed-*` and `vm-logs/m4-m5-{gcc,clang}-*`.
- Physical protocol execution remains NOT_RUN. No real controller was opened by the new tests. The ioctl has no returned byte count; fixed length/status/field checks are not proof of truthful firmware. Direct mapping is non-atomic and does not establish namespace-wide applicability, method access or authorization.

### M5 - Conditional platforms and truthful decline | PASS within available scope | 2026-10-28

- Reviewed the documented Windows 10+ property-query mechanism and MinGW enum/descriptor declarations. Native current MSVC plus fake-DeviceIoControl acceptance is unavailable. WN-01 is not enabled and no generic pass-through or untested alternate backend is compiled. Current native Windows execution remains NOT_RUN. Integrated MinGW Release all-target build PASS (process exit 0, final build target 100%); log: `vm-logs/m4-m5-windows-build.log`.
- Inspected macOS SDK 27.0 `NVMeSMARTLibExternal.h`: a GetIdentifyData interface exists, but selected IOMedia/controller association, user-client access and modern field validity were not established. No user client was instantiated. M-01 stays unapproved; this is not a claim that macOS has no Identify API. ATA/SCSI/bridges remain unapproved.
- Added `protocol_unavailable` testing the actual production decline adapter on all builds. PASS: 888 assertions covering cleared numeric output, no-query receipt, 24 unknown dimensions and forbidden request kinds. macOS Release/sanitizer portable suites PASS 15/15 each; integrated Linux full suites include this same test. No unavailable path claims hardware unsupported.

### M6 - Regression and distribution review | PASS available native scope | 2026-10-28

- macOS final Release and ASan/UBSan full CTest PASS 19/19 each, including nine probe CLI tests, unavailable adapter, existing native metadata integration and packaging. Logs: `m6-macos-{release,sanitize}-tests.log`. Expected unsupported-host CLI case SKIP; no physical protocol test was selected. A final test-only literal-selector option case then passed targeted CLI checks in both builds.
- Expanded CLI negatives with prohibited mutation command names and actual disposable file/directory/missing/false-alias selectors; no physical selector is used. All nine tests PASS on Mac Release and sanitizer and, from the byte-verified `m6-verification-source`, Linux GCC and Clang sanitizer binaries. Logs: `vm-logs/m6-verification-{gcc,clang}-probe-cli.log`.
- Source audit proves all 41 production/header/build files exactly match the integrated Linux-tested snapshot. Existing tracked regression tests and historical versioned progress/acceptance/release documents remain unchanged. Private receipt: `m6-source-audit.json`.
- Package checker now rejects duplicate/special/unsafe archive files and requires an executable Unix binary before its relocated smoke run. Mac package smoke PASS (35 exact files, relocated help/version). Private adversarial fixture PASS: 16 ZIP/TAR cases covering valid archives, duplicate files, symlinks, parent traversal, lost execute permission, wrong binary, missing and extra files. Receipt: `m6-package-faults.json`.
- Doxygen with warnings treated as errors initially FAIL: the existing `ds_json_device` header named its argument `device`, while its definition used `d`. Aligned only the declaration/documented parameter name with the existing definition. No ABI or behavior change. Final Doxygen HTML with the unchanged `WARN_AS_ERROR=YES` configuration PASS; log `vm-logs/m6-final-doxygen.log`. A diagnostic-only invocation listed the two related warnings before repair; it was not counted as passing acceptance. Historical reports were not changed to hide the failure.
- Rebuilt the repaired current source on Mac: Release and Debug ASan/UBSan full CTest PASS 19/19 each. Transferred `m6-final-source` with all 118 hashes verified; fresh Linux GCC Release and Clang Debug ASan/UBSan builds and full CTest PASS 24/24 each, including the latest nine CLI tests. Logs: `m6-final-macos-{release,sanitize}-tests.log`, `vm-logs/m6-final-{gcc,clang}-{configure,build,tests}.log`. The unsupported-host CLI subtest is expected SKIP on both native hosts; no configured native metadata group was skipped.
- Repeated all 38 default comparisons against the baseline using the final rebuilt fixture: PASS (`m7-default-compatibility.json`). Current code/header/build files match `m6-final-source` exactly; the earlier M6 audit predates only the documented parameter-name repair. Existing test sources and historical documents remain unchanged. Local Markdown links and private-path scan PASS (`m7-source-audit.json`).

### M7 - Candidate distribution and maintainer handoff | PASS with external gates | 2026-10-28

- Final MinGW Release all-target rebuild from `m6-final-source`: PASS. No Windows executable was run on the Unix host. Current native Windows/MSVC acceptance is NOT_RUN; no WN-01 provider is enabled. Log: `vm-logs/m6-final-windows-build.log`.
- CPack candidate checks PASS for all three platform archives: exact tested executable bytes and 35 allowlisted public files; ZIP/TAR integrity; native relocated help/version on Mac/Linux; Windows inspection with `--no-run`. Mac/Linux full test suites also include package smoke. Initial distribution logs: `m6-macos-package.log`, `vm-logs/m6-final-{linux-x86_64,windows-x86_64}-package.log`. No source, private dependencies, raw inventories or test logs are packaged.
- Delivery documents are snapshotted separately as `m7-delivery-source`. Every maintained-file hash is verified after transfer, and all 41 production/header/build files must match the final tested `m6-final-source` before using its binaries. Archives are regenerated after these final document edits. Final checks and hash receipts are external to the packaged documents: `m7-macos-package.log`, `vm-logs/m7-*-package.log`, `m7-delivery-sha256.json`, `m7-delivery-check.json` and distributed `SHA256SUMS`.
- Runtime inspection: Mac arm64 with deployment target 12.0 and system frameworks only; Linux x86-64 needs `libudev.so.1`/`libc.so.6`, maximum referenced glibc 2.34, no RPATH/RUNPATH; Windows x86-64 is a MinGW preview with system DLL imports. These are binary compatibility checks, not proof of execution on every supported OS release. No publisher-signing or notarization claim is made.
- Documentation/manifest/ASCII/API checks PASS: 118 maintained files, 438 C declarations/definitions and 86 Python functions/classes. Doxygen HTML generation PASS with warnings as errors after the documented repair. `git diff --check`, Markdown links, unchanged historical files/tests and public private-path scan PASS. Final documentation checks are repeated on the delivery snapshot.
- Source review confirms the only new production query is LN-01, fixed by the constructor and independently asserted in native fixtures. Default operations remain non-probing, positive support does not upgrade access/applicability/passthrough, failed identity cannot publish success, and authorization remains literal false. The manual non-system-device acceptance procedure is in TESTING; no physical query was executed.
- CI source review: existing portable/fault labels include the new fixtures on applicable jobs; native metadata jobs remain non-probing. Current committed-source CI is NOT_RUN because no final commit/push was authorized. Historical run 40546829221 is only baseline evidence. The maintainer must inspect jobs and skips for the exact future release SHA.
- Maintainer deliverables: `docs/RELEASE_NOTES_0.3.0.md`, `docs/ACCEPTANCE_0.3.0.md`, `docs/PROTOCOL_TEST_MATRIX_0.3.0.md`, `docs/RELEASE_CHECKLIST_0.3.0.md`, and the three archives plus SHA256SUMS in ignored `dist/v0.3.0/`. Repository HEAD remains `d00be042e9a64552b86b3fc807d2995e4110b165` on `main`, with the implementation uncommitted and the index empty. No commit, push, tag, release or repository setting was changed.

### Reproduction summary

- Mac: `cmake -S . -B build-v030 -DCMAKE_BUILD_TYPE=Release`, `cmake --build build-v030 --parallel 2`, `ctest --test-dir build-v030 -C Release --output-on-failure -V`. Sanitizer build uses `-DCMAKE_BUILD_TYPE=Debug -DDISKSAN_SANITIZERS=ON` and `build-v030-sanitize`.
- Linux: `cmake -S m6-final-source -B m6-final-gcc -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Release -DDISKSAN_WERROR=ON`; build and full CTest. Clang uses a separate build directory, `-DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Debug -DDISKSAN_SANITIZERS=ON`, `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.
- Windows cross-build: use `cmake/toolchains/windows-mingw.cmake`, Release and warnings as errors; build all targets, do not run them on Linux.
- Public checks: `python3 tools/check_documentation.py`, `doxygen Doxyfile`, `git diff --check`; CPack/check-package instructions are in DISTRIBUTION. Source hashes and complete compiler/test logs are retained privately under `.local-evidence/v0.3.0/`.

## Mandatory coverage distinctions

- Portable decoding fixtures are NOT evidence of device firmware behavior.
- Cross-compiled Windows code is NOT native execution.
- Hosted Windows tests are NOT evidence for arbitrary real disks or USB bridges.
- On-host protocol query output is NOT proof of erase authorization or future target identity.
- A reported `unsupported` method needs an explicit, valid, scope-matched hardware response; error codes and permission denial must remain unknown/unavailable.
- `SKIP` and `NOT_RUN` remain distinct from `PASS`; record exact test prerequisites.

## Completion and release decision

- IMPLEMENTATION_COMPLETE: all reachable, specified behavior implemented and its actual available regression/security tests passed, while limitations and blocked external evidence remain explicit.
- READY_TO_PUBLISH: separate maintainer determination after exact committed revision CI, artifact rebuild/hash checks and platform support claims have been reviewed. Do not assume implementation-complete implies released.
- Maintainer alone creates commits, tags, releases or deployment actions unless newly authorized.
