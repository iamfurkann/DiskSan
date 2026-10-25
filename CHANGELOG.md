# Changelog

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
