# Changelog

## 0.3.0 - Audited Read-only Capability Discovery (unreleased)

- Add explicit `capabilities <selector> --probe` without changing default metadata-only commands or schema-2 inventory contracts.
- Add one fixed Linux NVMe Identify Controller query for revalidated direct PCI namespace/controller mappings, with read-only access, no retries and bounded responses.
- Decode reviewed NVMe version and SANICAP fields into three independent support states; retain unknown access/applicability/passthrough and truthful unavailable/error reasons.
- Add opt-in receipts, controller-association revalidation, decoder/CLI/native request/fault fixtures and unsupported-platform adapter tests.
- Keep Windows/macOS/ATA/SCSI/USB protocol providers unavailable; no destructive command, raw pass-through or storage user-data access is introduced.
- Track actual candidate tests and external hardware/CI gates in `docs/PROGRESS_0.3.0.md`; historical entries below retain their preparation-state wording.

## 0.2.0 - Read-only Target Assessment (unreleased)

- Add `assess` and `capabilities` with a separate schema-2 envelope, stable policy
  findings, original evidence, explicit non-atomic identity limits and literal
  `erase_authorized: false`. Existing inventory commands and exit codes remain.
- Add selected-target revalidation, bounded observed topology, Linux proc/swap and
  loop-backing observations, macOS mount/volume metadata, and Windows zero-access
  volume-extents queries with conservative fallbacks.
- Keep six method families independently unknown for support, OS access,
  applicability and passthrough; no protocol commands or destructive backend exist.
- Add policy/target/parser/graph fixtures, CLI contracts, Linux/macOS native usage
  faults and live read-only assessment tests. Repair nested-document package checks.
- Native macOS/Linux results and Windows cross-build evidence are in
  `docs/PROGRESS.md`. Windows native runtime/MSVC and physical-controller coverage
  remain separate open gates; this change set has not been published.

## 0.1.1 - Reliability & Release Hardening (unreleased)

### Added

- Transactional identity reconciliation, allocation-failure sweeps, and bounded supplemental evidence in schema 2.
- Portable Windows descriptor/UTF-16 validation and synthetic boundary tests.
- Linux partial-scan and generation/revalidation fault fixtures.
- Push/PR CI for GCC, Clang sanitizers, AppleClang, and native MSVC; archive content/smoke checks; security policy.

### Changed

- Require consistent same-platform native identity and role before merging. Device numbers and serials alone do not establish identity.
- Include a valid Linux diskseq in snapshot identity and report when generation evidence is unavailable.
- Preserve USB transport with independently observed NVMe protocol evidence, while retaining unverified bridge/passthrough warnings.
- Separate portable, fault, native integration, and packaging test groups. Bracket macOS reference comparisons with bounded topology retries.

### Fixed

- Silent loss of duplicate-record aliases, warnings, relationships, and metadata sources.
- Acceptance of records after Linux opened-handle identity changes or device disappearance.
- Windows descriptor header offsets, unterminated/oversized interface paths, and disk/partition number inconsistencies.
- Build-directory RPATH changes between tested and packaged Linux executables.

Windows remains a preview pending native MSVC/runtime acceptance. CI is defined locally; hosted success requires the owner's push. No destructive API is introduced.

## 0.1.0 - Release candidate

- Add a shared C17 core, CLI, evidence/warning model, and safe JSON/text output.
- Discover Linux devices through libudev/sysfs and read-only capacity/sector queries; validate on a Debian VM.
- Discover macOS devices through IOKit/Disk Arbitration; distinguish APFS containers, volumes, and snapshots; validate on a real Mac.
- Add a Windows SetupAPI disk/partition and storage-query adapter; cross-compile with MinGW. Native runtime acceptance remains pending.
- Introduce JSON schema 2 with platform/native identity, nullable Unix numbers, and volume/snapshot roles. The earlier schema 1 draft was not released.
- Keep Windows records distinct when no Unix device number exists; never deduplicate by serial number.
- Add portable, native integration, sanitizer, and Linux ioctl/libudev fault-injection tests.
- Produce separate platform/architecture archives with CPack.
- Standardize all repository prose in English, document code with Doxygen `@brief`, and add an automated documentation check and contributor guide.

No erase, sanitize, format, discard, or erasure certification functionality is included. Native Windows acceptance, signing, and a tagged binary release remain pending. The repository uses the existing Apache License 2.0.

Historical note: the owner published v0.1.0 as a prerelease on 2026-10-25. The candidate notes above describe its preparation state.
