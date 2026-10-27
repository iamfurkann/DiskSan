# DiskSan 0.2.0 requirement acceptance and handoff

Status: WITH_LIMITATIONS for review; reported hosted Windows build FAIL, local correction awaiting MSVC rerun; not READY_TO_PUBLISH. This matrix maps
`docs/specs/v0.2.0.md` to source and executed evidence rather than treating a green
workflow definition as validation. Baseline HEAD is
`58d07c9a2f00f9a21c4e6c6a7ac96d9febe332bf`; the implementation is committed as
`407a5d2`. The subsequent Windows initialization correction remains local.

## Requirement audit

| Spec requirement | Implementation and authoritative check | Result |
|---|---|---|
| Sections 1-2: observational only; no destructive/storage-data path | Native usage source review plus exact allowlist in READ_ONLY_AUDIT_0.2.0.md; Linux flags/control allowlist wrappers, Mac/Windows fault seams; metadata-only capability provider | PASS source boundary; Windows native execution NOT_RUN |
| Section 3: portable facts, derived policy, method evidence separated | assessment.h/assessment.c, target_compare.c, usage.c, capabilities.c; native OS headers remain in adapters | PASS, policy/target/graph fixtures |
| Section 4: new CLI/help/JSON/stdout/exit behavior; old commands preserved | main.c, shared ds_json_device renderer, assessment_output.c; 11 assessment_cli tests, 18 old output tests and 5 old CLI tests | PASS on Mac/Linux; Windows execution NOT_RUN |
| Section 5: fresh selection, node types, aliases, known identity/state/generation changes | target.c calls original selector at both ends; target_compare.c; 617 target assertions, graph checks, native alias/root tests, existing node-type/devnum/diskseq tests | PASS accessible scope; physical hotplug/ABA not guaranteed |
| Section 6 Linux: observed partition/holder/slave/virtual/backing graph, mounts, swap, root and namespace limits | Bounded usage graph, pure proc parsers, stat-only swap/loop mapping, closing graph comparison; 622 parser/181 graph assertions, native Linux and usage faults | PASS; hidden/layered/backing-inode scope explicitly unknown |
| Section 6 macOS: observed registry/volume/mount/system relationships and APFS uncertainty | Disk Arbitration/getfsstat/stat with registry snapshots; native root/alias checks and 474 synthetic usage-fault assertions | PASS on available Mac; multi-device APFS/Fusion/RAID NOT_RUN |
| Section 6 Windows: read-only storage/volume queries and truthful restrictions | Zero-access GUID handles, bounded UTF-16/MULTI_SZ, two extent queries and selected-identity recheck; 16 prepared query-fault scenarios | PASS corrected MinGW build; reported MSVC build FAIL, corrected MSVC/native faults/live acceptance NOT_RUN |
| Section 7: independent method support/access/applicability/passthrough, no guessed capability | Six methods, metadata-only provider, four independent unknown dimensions; synthetic unknown/supported/unsupported/unavailable/conflict tests | PASS; protocol/hardware support deliberately not queried |
| Section 8: stable findings, hazard precedence, unknown-safe inspection state | Fixed rule taxonomy, 1,291 assertions including every hazard truth combination; literal false authorization and end-of-observation expiry | PASS |
| Section 9: bounded ownership, Unicode, errors, conservative partial scans, no new runtime dependencies | Fixed evidence/graph/extent bounds; existing transactional metadata allocation sweep; new calloc/stream/loop-retention fault paths; UTF-16/MULTI_SZ and output contracts | PASS available tests; Windows native response execution NOT_RUN |
| Sections 10.1-10.4: policy, resolver, parser, capability negative fixtures | assessment_policy, target_window, usage_parsers, usage_graph, assessment_cli, native usage faults | PASS available platforms; Windows fault executable compiled only |
| Section 10.5: memory/errors, cleanup, repeated reset, bounds, stream failures, uint64/UTF-8 | identity_faults, assessment_policy, assessment_cli, output_contract, Linux/macOS native usage-fault fixtures | PASS available platforms |
| Section 10.6: old/new CLI regression and no success payload on fatal errors | Separate old inventory and new assessment fixtures; exit classes 1/2/3/4, failed/unevaluated renderer refusal | PASS Mac/Linux |
| Sections 10.7-10.8: native, compilers, sanitizers, docs and packages | Recorded source snapshots and commands below; no blanket skip or continue-on-error | PASS recorded Mac/Linux checks; hosted Windows build FAIL, corrected MSVC/native runtime NOT_RUN; archives need refresh |
| Section 10.9: read-only trace/review | Source-level call review and deterministic allowlist fixtures; no full native syscall trace claimed | PASS review; Windows native execution NOT_RUN |
| Section 10.10: partial evidence never authorizes erasure | Literal false JSON/text; unknown-safe policy, poisoned-result refusal, incomplete native scope findings | PASS |
| Sections 11-12: milestones, accurate docs/manifest/version/handoff | PROGRESS.md, updated user/developer/schema/security/release docs, source version 0.2.0; documentation checker and final diff review | PASS source/docs handoff; previous platform archive checks predate the Windows initialization correction |

## Executed verification

- Mac arm64 AppleClang Release: 15/15 CTest groups PASS for final source behavior.
- Mac arm64 AppleClang ASan/UBSan: 15/15 PASS, including native integration and
  synthetic native usage cleanup/allocation faults.
- Linux x86-64 VM GCC Release: 18/18 PASS for `m6-final-source`, an isolated
  89-file snapshot with every SHA-256 equal to the Mac source copy.
- Linux Clang ASan/UBSan: final source 18/18 PASS; native assessment and all new Linux fault paths included.
- Windows x86-64 MinGW: M6 all-target build PASS, including production adapter and
  `test_windows_usage.exe`; the final shared-system-directory API correction and
  its fault object passed a separate M8 production/test cross-build. No Windows executable was run on Linux or macOS.
- Maintainer-supplied hosted Windows build log: FAIL, MSVC C4701/C2220 for the
  conditional closing-query buffer in `windows_usage_fault`. Explicit initialization
  is now applied. M9 all-target MinGW Release cross-build: PASS after verifying all
  90 transferred source hashes. Corrected hosted MSVC build/tests: NOT_RUN.
- Post-correction Mac portable regression: Release 11/11 and ASan/UBSan 11/11 PASS.
  These checks do not compile or execute the Windows usage adapter.
- Documentation/manifest/ASCII and `git diff --check`: PASS including this 90-file maintained manifest.
- M7 package smoke: Mac/Linux relocated checks and Windows content/CRC checks with
  `--no-run` passed for that snapshot. Those archives and checksums predate the
  initialization correction and updated documents; regenerate before publication.

The existing CLI unsupported-host case is an expected SKIP on supported systems.
No target/alias/root prerequisites were skipped in the executed Mac/Linux native
assessment checks. Windows native tests, hosted MSVC verification of the local
correction, physical-controller/bridge/hotplug and older-OS/other-architecture gates
are NOT_RUN. Other hosted job results were not independently checked for this fix.
Full commands, repaired failures and private-log names are in PROGRESS.md. Mac/Linux/core hashes are unchanged from the fully tested M6 behavior; code corrections are Windows-only.

## Explicit limits and maintainer actions

1. **Windows validation pending; corrected MSVC/runtime NOT_RUN.** On the
   owner's Windows device, use a normal terminal and run the source build/test
   commands in TESTING.md, including windows_usage_faults and assessment_native.
   Run manual CLI/JSON and permissions checks from RELEASE_NOTES_0.2.0.md before
   describing Windows as validated. No elevation is required to make a test pass.
2. **Reported hosted Windows build FAIL; corrected revision NOT_RUN.** After review
   and the owner's commit and push, observe the new GitHub Actions revision/results. Existing workflow
   labels automatically select the new portable/fault/integration tests; workflow
   definitions were not changed or dispatched during this work.
3. **Physical hardware matrix NOT_RUN.** Validate relevant real USB/SATA/NVMe/SCSI,
   storage stacks and bridge identity with metadata-only tests as hardware becomes
   available. Unknown capability evidence is intentional until an independently
   audited read-only protocol provider exists; this is not an erase feature.
4. **Publication requires maintainer review/authorization and refreshed assets.**
   The agent did not stage, commit, push, tag, change settings, dispatch CI or publish
   this correction. Regenerate archives/checksums after final validation. Candidate title: `DiskSan v0.2.0 - Read-only Target Assessment`;
   proposed future tag: `v0.2.0`. Signing/notarization is not claimed.

Release readiness is WITH_LIMITATIONS for local review, not READY_TO_PUBLISH.
Windows preview status and outstanding native/hosted/hardware gates must remain
visible in any eventual release. See DISTRIBUTION.md for per-platform packages
and RELEASE_NOTES_0.2.0.md for exact non-destructive manual acceptance steps.
