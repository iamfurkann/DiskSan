# Codex Goal - DiskSan v0.3.0

The following is a *new* objective. Do not reuse the old v0.2.0 `/goal`; that milestone is published. This file is a human launch document; placing it in `docs` does not by itself activate a thread-level goal.

## Ready-to-paste `/goal`

Copy the one line inside the code block, starting with `/goal`, into a Codex thread if that interface supports Goals:

```text
/goal Implement DiskSan v0.3.0 Native Protocol Capability Discovery from the actual current repository checkout according to AGENTS.md, docs/specs/v0.3.0.md and docs/PROTOCOL_QUERY_AUDIT_0.3.0.md. First inspect HEAD, working-tree changes, the published v0.2.0 baseline and existing tests; preserve user work. Add a closed, precisely audited and explicitly opt-in read-only `capabilities <selector> --probe` path, with genuine capability evidence from only verified fixed NVMe Identify Controller query mechanisms, primary Linux and conditional Windows; do not invent ATA/SCSI/macOS support or use generic pass-through. Preserve default non-probing list/info/assess/capabilities behavior, schema-2 compatibility, independent capability dimensions, non-atomic identity limits and literal erase_authorized=false. Independently design, implement, test and repair coherent milestones M0-M7, including all forbidden-command/flag tests, bounded response decoding, target-revalidation and handle/error cleanup, platform tests, sanitizers, documentation/manifest, and packaging checks. Keep docs/PROGRESS_0.3.0.md current with actual evidence and distinguish PASS/FAIL/SKIP/BLOCKED/NOT_RUN; do not paste private identifiers into public files. Continue reachable independent engineering work without routine confirmation requests, but never read/write storage user-data sectors, issue any state-changing protocol command, elevate privilege for tests, auto-probe physical hardware, or stage/commit/push/tag/release/change repository settings without new authorization. If platform API or physical device access is unavailable, implement safe truthful fallbacks, leave unsupported claims UNKNOWN/UNAVAILABLE, and report exact blockers. Finish with an evidence-based candidate handoff, not an unsupported release or v1.0.0 claim.
```

## Normal prompt alternative (when `/goal` is not supported)

```text
Continue development of DiskSan from its live repository state. Implement v0.3.0 as described in AGENTS.md, docs/specs/v0.3.0.md and docs/PROTOCOL_QUERY_AUDIT_0.3.0.md. First inspect the actual branch, HEAD, user edits and current tests. Preserve existing v0.2.0 behavior and historical documents. Execute M0-M7 with real implementation and targeted/full tests, not just a plan. Add opt-in, strictly allowlisted read-only native NVMe capability discovery where safely supported; leave unverified ATA/SCSI/macOS/bridge cases unknown or unavailable. Never use destructive commands or automatic live-device probing. Update docs/PROGRESS_0.3.0.md with verified results, skips and limitations. Do not stage, commit, push, tag, publish releases or change repository settings. Give a concise technical report listing implemented code, exact tests, unresolved physical/native gates and the proposed next safe step.
```

## Optional short session continuation

If your thread context is reset, reopen the actual repo and send:

```text
Continue DiskSan v0.3.0 from the current checkout. Read AGENTS.md, the next incomplete milestone in docs/PROGRESS_0.3.0.md, its relevant sections in docs/specs/v0.3.0.md and the native query audit. Check git status and existing user edits before changes. Complete and test the next reachable milestone, update actual evidence, and continue as permitted; do not reset to v0.2.0 or publish anything.
```

## Operational notes

- Some Codex versions/clients may not provide `/goal`; use the ordinary prompt instead. Do not assume an unrecognized slash command enabled a persistent goal.
- The goal is associated with the current thread if supported; it is not automatically global across projects.
- `AGENTS.md` should be at the repository root for standing instructions. The version spec and query audit contain functional requirements, and `PROGRESS_0.3.0.md` carries persistent facts.
- Actual build/test/CI evidence beats any status claim in old plans. Ask the maintainer only when new privileges, hardware, authorization, missing dependency or substantial scope change is required.
- Completing implementation is not permission to run dangerous commands or create a GitHub Release.
