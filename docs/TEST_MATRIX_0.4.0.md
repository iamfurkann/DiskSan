# DiskSan v0.4.0 - evidence and test matrix

A PASS is a real test execution within the stated scope. The handoff report identifies the M6 standalone CLI source and the later M7 desktop source. All standalone CLI build and test inputs match M6. Commands and skipped cases are recorded in `docs/PROGRESS_0.4.0.md`.

| ID | Acceptance area | Required evidence | Status |
|---|---|---|---|
| T01 | Baseline freeze | Commit/dirty tree, 0.3.0 command contracts, git status, baseline CMake/CTest | PASS with recorded baseline SKIP |
| T02 | Plan risk policy | True/false/unknown truth tables, conflicting evidence, mounted/system/swap/RAID/role, identity changes | PASS: M3 Mac/Linux, 14,206 policy/output assertions |
| T03 | No execution | Link-level forbidden-call fixture, exhaustive command-dispatch tests, no native read/write/sanitize path | PASS: M3 CLI tripwire and command rejection; M6 object-symbol and frozen Git-source audit passed |
| T04 | Plan CLI | Options `--`, invalid forms, opaque selectors, correct exit codes, JSON schema-2 compatibility | PASS: M3 Mac/Linux CLI suites |
| T05 | Simulation | Deterministic step ordering, blocked and unknown cases, no irreversible/verified states, `simulation_only` always true | PASS: M3 deterministic model transcript tests |
| T06 | Output & resources | UTF-8, uint64, JSON escaping, truncated stdout, allocation fault, invalid evidence, multi-run cleanup | PASS: M3 faults/output; M5 Mac JSON/process boundaries |
| T07 | v0.3.0 regression | All old portable/integration/probe forbidden tests; `capabilities --probe` still opt-in and byte-identical native allowlist | PASS: M3 old groups and 39 compatibility cases; M6 Mac/Linux regression reruns passed |
| T08 | Linux | Native GCC Release, Clang ASan/UBSan and read-only integration against accessible devices | PASS: M6 26/26 each, GCC Release and Clang ASan/UBSan, including native assessment/discovery |
| T09 | macOS | Native AppleClang Release and sanitizers, read-only assessment integration | PASS: M7 26/26 each, Release and sanitizer with five desktop groups |
| T10 | Windows | Native MSVC C17 Release, deterministic faults and native integration, record skipped prerequisites | NOT_RUN: current native MSVC; MinGW compilation only |
| T11 | Qt optional-off | Configure/build/test without Qt anywhere, CLI archives lack GUI dependencies | PASS: Mac forced-no-Qt configure/build and no Qt linkage; 47-file Mac CLI package passed with desktop enabled |
| T12 | Qt optional-on | Build `disksan-desktop` from actual Qt; launch UI/offscreen smoke (if supported) | PASS: Mac Qt 6.12 actual runtime; Debian 13 Qt 6.12 Release and sanitizer offscreen/native inspection PASS, Windows NOT_RUN |
| T13 | UI data bridge | Only approved argv, fixed program path, JSON validation, errors, timeout, output bounds, stale selection cancellation | PASS: Mac and Debian Release/sanitizer child-process and schema tests, including failed-start supersession |
| T14 | UI UX | Empty/error/unknown states, selected-device assessment, simulation banner, keyboard navigation, "DEMO DATA" watermark | PASS: Mac and Debian actual QML fixture tests; end-to-end screen readers NOT_RUN |
| T15 | Docs/packages | Manifest, ASCII, Doxygen, `git diff --check`, CI on exact SHA, CLI package smoke/checksums | Local PASS: 156-file checker, Doxygen, diff check, exact 47-file packages for all three targets and SHA256SUMS. Hosted CI on the final commit is NOT_RUN. |
| T16 | External hardware | Real NVMe/USB/SATA/SCSI and physical hotplug/ABA (not required to simulate); explicitly NOT_RUN unless independently tested | NOT_RUN: no physical protocol/hotplug acceptance in this work |

## Guard tests

Add a fixture adapter that aborts immediately if any 0.4.0 planning/simulation/GUI call would execute `ds_native_protocol_probe`, a generic ioctl or a mutation function. Test that the old standalone `capabilities --probe` remains explicitly opt-in and never reachable from desktop callbacks. Include a machine-readable static/source review of new native calls but do not call that review a proof of runtime safety.

## Evidence recording template

Record `Date | SHA/source snapshot | OS/toolchain | command | passed/failed/skipped | log path | limitations`. Distinguish CTest aggregate PASS from skipped unittest subcases (especially on Windows runners). If GUI is not installed in CI, mark its tests NOT_RUN and retain a separate GUI acceptance gate. No guessed numbers or inherited historic PASS.
