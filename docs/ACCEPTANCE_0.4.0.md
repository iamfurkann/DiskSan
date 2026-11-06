# DiskSan v0.4.0 - requirement acceptance and handoff

Status: IMPLEMENTATION_COMPLETE / WITH_LIMITATIONS. The [handoff](WORK_REPORT_0.4.0.md) identifies the exact tested source and artifacts. A10 remains a maintainer release gate.

| Gate | Required deliverable | Acceptance proof | Current evidence |
|---|---|---|---|
| A0 | Live baseline and worktree preserved | Compare 0.3.0 tag/current source, local baseline regression | PASS: preserved base/worktree; recorded baseline integration SKIP |
| A1 | Pure C plan model with conservative risk precedence | Portable risk, conflicts, overflow and ownership fixtures | PASS: Mac/Linux Release and sanitizer policy tests |
| A2 | `plan` and `simulate` additive CLI/JSON | Full new CLI/JSON tests, no regression in old schema and selectors | PASS: Mac/Linux CLI fixtures; 39 legacy comparisons |
| A3 | No operational path | Static allowlist audit and forbidden native-call fixtures; false authorization | PASS: frozen protocol hashes and live synthetic tripwire; the eight files also matched the base Git objects |
| A4 | Design-only threat and authorization model | TOCTOU/ABA, controller scope, root/shared consumers, privacy, privilege and recovery reviewed | Implemented and reviewed design-only model; no service/token/executor |
| A5 | Optional working Qt Quick UI | Actual native Qt build, list/select/info/assess/capability/plan/simulate views | PASS: macOS Qt 6.12 Cocoa/offscreen native inspection; Debian 13 Qt 6.12 Release and sanitizer offscreen/native inspection PASS, Windows NOT_RUN |
| A6 | Desktop untrusted process/JSON boundary | Process allowlist, output bounding, race/error/stale-handling, no probes, no shell | PASS: Mac and Debian Release/ASan/UBSan parser, contract and actual child-process tests |
| A7 | Accessibility and truthfulness | Loading/empty/unknown/error/demo labels, keyboard navigation and no execute control | PASS: Mac and Linux QML fixture keyboard/accessible names/high-contrast; screen readers NOT_RUN |
| A8 | Cross-platform native regressions | Linux GCC/Clang sanitizer, Mac AppleClang, Windows MSVC supported jobs | Mac full suite and Linux standalone CLI PASS 26/26 per compiler mode; MinGW cross-build PASS; native MSVC NOT_RUN |
| A9 | Documentation/packaging/licensing | CMake optional-off build, manifest/ASCII/API/Doxygen/package checks, Qt license/OS matrix | PASS: forced-no-Qt and explicit missing dependency checks, Doxygen, manifest and three verified 47-file CLI archives with checksums. GUI binary distribution remains open. |
| A10 | Release gates | Reviewed commit, tagged SHA CI, optional GUI artifact proof, maintainer authorization | NOT_RUN: no commit/tag/current hosted run or publication approval |

A real device erasure test cannot satisfy a gate for 0.4.0; it violates this version's non-destructive boundary. A GUI that is not compiled/runnable in the maintainer's Qt environment does not satisfy A5 even if QML source files exist. No Qt dependency in default CLI build. Inspect all skip statements before calling a native platform validated.

Final report: exact source SHA, new files, commands/results, QML smoke runtime, safe invocation audit, known unsupported OS/Qt versions, hardware limitations and outstanding review. Use `WITH_LIMITATIONS` for incomplete physical hardware (as expected), and record optional Qt build absence explicitly. Do not claim READY_TO_PUBLISH without maintainer release approval.
