# DiskSan v0.2.0 baseline and evidence ledger

## Public repository checkpoint

Research date: 2026-10-27, repository `iamfurkann/DiskSan`.

- Published annotated tag: `v0.2.0`, pointing to commit `d00be042e9a64552b86b3fc807d2995e4110b165`.
- Release: `https://github.com/iamfurkann/DiskSan/releases/tag/v0.2.0`, marked pre-release. Three OS archives and `SHA256SUMS` are published. The release title was corrected to `DiskSan v0.2.0 - Read-only Target Assessment`.
- Tag-triggered GitHub Actions run `40546829221` for the exact tagged SHA: all four jobs passed (Linux GCC Release, Linux Clang ASan/UBSan, macOS AppleClang Release, native Windows MSVC C17 Release). Run: `https://github.com/iamfurkann/DiskSan/actions/runs/40546829221`.
- Windows hosted integration and synthetic metadata tests execute, but `assessment_native` contains a skipped Windows system-volume/root-dependency subtest. Real physical hardware and diverse Windows disk topologies remain unverified.
- Source CMake version: `0.2.0`; JSON schema version remains `2`.
- This is an external observation, not evidence that the maintainer's local working tree exactly matches the published tag. Before implementation, inspect branch, HEAD, status, pending changes, and relevant tests. Preserve everything unrelated.

## Existing behavior to preserve

- `list`, `info`, `assess`, `capabilities` (including JSON); `--all` is limited to list. Existing 0/1/2/3/4 exit classes and output/error boundaries remain documented in current source.
- `src/core/target.c`: re-resolves the original selector before/after native usage and capability observations; detects observable identity changes but does not provide atomic or future authorization.
- `src/core/assessment.c`, `src/core/capabilities.c`: six method families, four independent dimensions each: `support`, `os_access`, `applicability`, `passthrough`. Native provider is metadata-only and reports all method dimensions `unknown`. Synthetic tests exercise other states.
- `src/platform/linux/usage.c`: bounded proc/sysfs mount, swap, holder/slave and loop-backing observation with limited namespace scope; no block user-data reads.
- `src/platform/macos/usage.c`: IOKit/Disk Arbitration mount information and incomplete APFS/physical scope.
- `src/platform/windows/usage.c`: zero-access volume extent and path queries, explicit response bounds and persistent topology uncertainty. Extent buffers were initialized in the tagged commit to resolve MSVC C4701.
- `src/output/assessment_output.c`: separate schema-2 operation envelope, observations/findings/method evidence; always literal `erase_authorized:false`, identity future guarantees false, inspection expiry at observation end.
- `docs/PROGRESS.md` and `docs/ACCEPTANCE_0.2.0.md` preserve historical development-phase test claims. Some text predates tag CI and publication; do not mistake older `NOT_RUN` entries for evidence that the later tagged runner did not execute.

## Baseline gaps and constraints

1. No genuine native NVMe/ATA/SCSI protocol Identify/Inquiry/Log capability provider exists; all live method dimensions are `unknown`.
2. No tested device-wiping backend, verification certificate, or policy authorization. This is deliberate.
3. Controller/namespace, USB bridge, virtualization, multipath, APFS, Storage Spaces and hidden usage remain incompletely observable.
4. Windows MSVC CI passed on a hosted runner, but owner real-hardware acceptance and Windows root assessment remain open.
5. Published unsigned packages and tag do not constitute cryptographic publisher attestation. Do not claim independently verified distribution reproducibility without comparing downloaded bytes and hashes.

## v0.3.0 initial handoff

Preserve the complete 0.2.0 public behavior and use `docs/specs/v0.3.0.md` for new version scope. If the maintainer has newer local commits or public changes after this checkpoint, review and update the baseline notes before implementation; never roll them back to match this historical SHA.
