# DiskSan v0.3.0 - requirement-to-evidence acceptance

Status: IMPLEMENTATION_COMPLETE, WITH_LIMITATIONS. PASS below applies only to the named synthetic or native-software scope. Physical queries, current MSVC execution and final committed-source CI are NOT_RUN. READY_TO_PUBLISH is a separate maintainer decision and is not claimed.

| Requirement | Implementation evidence location | Verification gate | State |
| --- | --- | --- | --- |
| Default read-only non-probing CLI unchanged | `src/main.c`, `src/core/target.c`, `tests/probe_forbidden.c` | 38 baseline comparisons; unchanged old fixtures abort if the new provider is called; `probe_cli` environment/default checks | PASS |
| Explicit `capabilities --probe` grammar | `tests/test_probe_cli.py`, `tests/test_probe_target.c` | Option order, literal `--probe` after `--`, duplicates, prohibited commands/options, disposable invalid selectors, exit classes | PASS |
| Exact native allowlist LN-01 | `protocol_query.c`, `test_linux_protocol_query.c`, `test_linux_protocol.c` | 37,694 constructor assertions; all 72 request bytes, 4096 initialized reply bytes, single syscall, exact open flags; 255 forbidden enum cases | PASS synthetic; WN-01 NOT_APPROVED |
| Fixed bounded read-only response | `src/core/protocol.c`, `tests/test_protocol.c` | 12,157 assertions: lengths 0..4095, oversized/null/unaligned, endian/version/reserved fields, 1,024 arbitrary-buffer trials | PASS; ioctl returned-byte count unavailable |
| Controller/namespace target linkage | `protocol_mapping.c`, native lifecycle fixture and target window | Fresh mappings before open/query and after query, changed generation/node/parent/NSID/CNTLID, closing selector validation | PASS synthetic; physical NOT_RUN |
| Preserve schema 2 and device model | `docs/DEVICE_MODEL.md`, assessment renderer, old/new CLI fixtures | 38 default byte comparisons after version normalization; additive receipt only on explicit opt-in | PASS |
| Six methods x four independent dimensions | `protocol.c`, `probe.c`, policy/decoder/provider fixtures | Only three support dimensions observed; all remaining dimensions unknown; permission denial is unavailable access, mixed evidence/overflow fail closed | PASS |
| Always false erase authorization and expiring snapshot | Existing policy and serializer; `test_probe_cli.py` | Positive synthetic support preserves false authorization, non-atomic identity and end-of-observation validity | PASS |
| Unsupported OS/device provenance | `protocol_unavailable.c`, Linux eligibility gates | 888 assertions on actual decline adapter; non-direct/unknown/bridge/observed-layered cases decline without native calls | PASS synthetic |
| Failure safety and cleanup | Linux lifecycle fixture, target and CLI fixtures | 421 lifecycle cases / 279,111 assertions; open/query/close/OOM/status/identity faults; output failures and cleared rejected results | PASS |
| Standard platform/regression matrix | [Protocol test matrix](PROTOCOL_TEST_MATRIX_0.3.0.md) | Mac 19/19 Release and sanitizer; Linux 24/24 GCC and Clang sanitizer; MinGW all-target build | PASS available scope; current MSVC/CI NOT_RUN |
| Distribution/doc quality | README, CHANGELOG, TESTING, model, manifest, release docs | Manifest/ASCII/API, warning-as-error Doxygen, 16 archive fault cases, three exact-content packages and native Mac/Linux smoke; delivery hashes external to packaged docs | PASS candidate scope; committed CI association NOT_RUN |
| Safety review/hardware claims | [Query audit](PROTOCOL_QUERY_AUDIT_0.3.0.md), manual procedure in TESTING | Only LN-01 active; no live physical query executed; all physical claims remain unverified | PASS scope disclosure; physical NOT_RUN |

## Candidate evidence and external gates

The base remains `main` at `d00be042e9a64552b86b3fc807d2995e4110b165`.
There is no final commit or tag: the candidate is an uncommitted working tree.
Initial user changes were preserved; historical 0.2.0 reports and existing
regression test sources were not rewritten. The missing historical specification
was restored exactly from the baseline tag.

The [progress ledger](PROGRESS_0.3.0.md) records immutable source snapshots,
commands and private log names. Final Linux GCC/Clang/MinGW binaries were built from
`m6-final-source`; all 41 production/header/build files match the current
candidate. Final full suites include the latest Python CLI tests and the API
parameter-name documentation repair. Later delivery documentation checks are
recorded separately rather than attributed to the earlier full suite.

LN-01 is the sole ACTIVE protocol query: Linux NVMe controller Identify,
opcode 0x06, CNS 1, NSID 0, 4096 bytes, timeout 5000 ms, read-only descriptor.
The driver may deny it; no elevation, writable retry or alternate alias follows.
The kernel ioctl returns status, not a transferred-byte count. Fixed initialized
buffers and validated fields do not prove truthful firmware or atomic identity.

WN-01, macOS, ATA, SCSI and bridge queries are NOT_APPROVED. Windows native
MSVC/API acceptance is unavailable locally; its production adapter performs no
protocol call. macOS has a public Identify interface, but the required target
association and modern response contract are not established. Physical NVMe
acceptance requires a maintainer-selected non-system device. Current hosted CI
requires a maintainer-created and pushed commit; historical 0.2.0 CI is not a
substitute. These are external release gates, not passing tests.

## Required final report fields

- Base and final Git commit SHA, branch, dirty-file status and all user changes preserved.
- Implemented source modules plus request allowlist status: ACTIVE vs NOT_APPROVED vs BLOCKED.
- Every command actually run, OS/compiler/runtime, counts, successes, fails, skips and relevant exact logs retained privately.
- Native API/header version constraints and unsupported transports/bridges, controller scope limits, and unverified hardware.
- Evidence that *default* commands do not issue protocol queries, and the opt-in path has no mutation or writable-handle behavior.
- Manifest/ASCII/Doxygen/packaging checks and changes to release notes.
- Whether the result is IMPLEMENTATION_COMPLETE, WITH_LIMITATIONS, BLOCKED, or READY_TO_PUBLISH; this last status is exclusively a maintainer release decision.

Do not mark external v0.2.0 CI as v0.3.0 validation. Release candidate readiness is separate from finishing reachable coding tasks.
