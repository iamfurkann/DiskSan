# DiskSan - Codex repository instructions

## Scope and engineering authority

DiskSan is a C17/CMake storage inspection and, in later releases only, sanitization project for Linux, macOS and Windows. The active 0.3.0 objective is **opt-in, audited, read-only native protocol capability discovery**. Version 0.3.0 MUST NOT erase, sanitize, format, discard, change partitions or filesystems, unlock security features, set firmware features, mount/unmount anything, change power states, or read/write storage user-data sectors.

You may independently research, design, edit relevant repository files, refactor, add tests, and execute safe verification. Make routine engineering decisions autonomously. Seek user input only for a real missing permission, inaccessible hardware, a scope-changing product decision, or a safety boundary that cannot be proved.

Do not commit, stage, push, tag, publish GitHub releases, change repository settings, trigger remote publishing, or modify unrelated repositories without explicit authorization. Do not use elevated privileges or relax a security control to make a test pass. Preserve existing local user changes. Keep real device inventories, serials, logs and personal paths out of the public repository.

## Documents and version separation

- `docs/specs/v0.3.0.md`: authoritative **current** version requirements and acceptance criteria.
- `docs/BASELINE_0.2.0.md`: historical/public snapshot and validation scope; inspect the actual checkout at start and do not treat historical notes as current proof.
- `docs/PROGRESS_0.3.0.md`: current milestone ledger, observed tests, unverified targets, and handoff.
- `docs/PROTOCOL_QUERY_AUDIT_0.3.0.md`: maintain an exact per-command/query allowlist with permissions, input/output bounds, status handling, and negative tests **before enabling any new native query**.
- `docs/ROADMAP_TO_1.0.0.md`: direction, not permission to implement destructive features in 0.3.0.
- Preserve existing 0.2.0 `docs/PROGRESS.md`, `docs/specs/v0.2.0.md`, acceptance reports and historical release files. Do not silently rewrite version history.

Read only the sections relevant to the active milestone. Avoid repeatedly dumping entire histories or full test logs into context. The existing code and actual test outcomes prevail over speculative guidance.

## Non-negotiable read-only protocol boundary

- Discovery and assessment snapshots are non-atomic and never authorize erasure. `erase_authorized` remains literal JSON false; no authorization token, backend command executor, or device-changing API may be introduced in 0.3.0.
- No generic user-controlled opcode, feature identifier, vendor command, pass-through command blob, ioctl number or raw pointer submission. An allowlist must specify **concrete**, individually audited query structures and fixed direction/length limits. Reject unknown requests before any device handle is opened.
- Probe invocation must be explicit and safe by design. Ordinary `list`, `info`, `assess`, `capabilities` and existing JSON behavior must remain non-probing/unchanged. If native probing is implemented, use a separate opt-in `capabilities <selector> --probe` path and keep old invocations backward compatible.
- Per-platform adapters must prove that each chosen query is observational under its specific OS driver interface. A command family name such as admin pass-through or SG_IO is NEVER a sufficient safety claim. No firmware update, namespace management, self-test initiation, security-state mutation, sanitize or erase commands.
- Query success is not proof of a method being executable, permitted or safe. Keep support, OS access, applicability and passthrough independent. Unknown/unavailable are distinct from hardware unsupported. USB bridges, shared controllers, namespaces, RAID, virtualization and opaque transport retain scope limitations.
- No automatic live-hardware tests, no elevation fallback, no automatic opt-in from the environment or test framework. All physical device checks require a maintainer-selected device and explicit deliberate invocation; never use a boot/system disk as a convenient test target.
- Fail safely on mismatched identity, ambiguous target, response truncation, unrecognized version, unexpected transport, permission error, or incomplete protocol evidence. Never turn a failed probe into observed_unsupported or into a successful permission verdict.

## Implementation and verification style

- Portable policy/decoding logic belongs in `src/core` or a platform-neutral module; OS-specific requests and handles belong under `src/platform/<os>`. No platform native types in public portable contracts. C17 and existing ownership conventions apply.
- Follow `docs/CODING_STANDARD.md`: English and ASCII repository content, file/function Doxygen `@brief`, public API parameter/return documentation, bounded data, explicit errors, no spurious capability certainty. Update `docs/FILE_MANIFEST.txt` for every maintained file added/removed; keep the manifest sorted and complete.
- Work through coherent verified milestones. Implement, run targeted tests and fix failures before changing to another milestone. Run broader regressions at integration points, then full Release and sanitizer tests on available hosts. Run `python3 tools/check_documentation.py`, `git diff --check`, and applicable packaging checks.
- New protocol code requires deterministic synthetic request/response fixtures, short/oversized/invalid buffer cases, endian/bit-field decoding tests, failed open/ioctl/close paths, changed-device/freshness tests, and an independent safety allowlist assertion. There must be tests proving forbidden commands and write-access flags are impossible from the exposed CLI.
- Observe actual hosted GitHub Actions for the final commit, not merely YAML syntax. Do not equate native Windows runner tests with dedicated storage-hardware acceptance. Do not fabricate macOS protocol access when the platform lacks a supported public API.
- With the maintainer's macOS-to-Linux-VM workflow, use actual observed paths and isolated DiskSan workspaces; preserve other projects and verify transferred source bytes. Do not use `--delete` or assume remote addresses.
- Record `PASS`, `FAIL`, `NOT_RUN`, `SKIP`, or `BLOCKED` in `docs/PROGRESS_0.3.0.md` after each verified milestone; include test command/snapshot and remaining gaps, not raw sensitive inventories.

## Release behavior

Completing implementation does not mean publishing. The maintainer controls release authorization, tag and binary distribution. Accept partial platform capability support with accurate coverage labels; never upgrade a scope-limited preview to a verified capability. Do not advance version to 1.0.0 or implement real storage mutation from these instructions.
