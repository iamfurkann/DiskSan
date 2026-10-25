# DiskSan v0.1.1 - Reliability & Release Hardening

Prepared release notes; publication is a maintainer action. DiskSan remains a read-only C17 device discovery CLI for Linux, macOS, and Windows.

## Added

- Transactional duplicate reconciliation, bounded supplemental metadata evidence, and deterministic allocation-failure regression tests.
- Linux generation/partial-inventory fixtures and portable Windows descriptor/UTF-16 boundary tests.
- Push/PR CI definitions for GCC, Clang sanitizers, AppleClang, and native MSVC.
- Exact archive allowlist/executable checks, relocated CLI smoke tests, and a security policy.

## Changed

- Matching native identity, platform, role, and consistent device numbers are required before merging. Ambiguous identities fail explicitly.
- Linux diskseq participates in snapshot identity when available; missing generation evidence is reported.
- Independent USB/NVMe evidence retains USB transport without asserting physical media or passthrough support.
- Schema 2 gains an additive evidence array; existing fields, null values, exact capacities, and exit codes are preserved.

## Fixed

- Silent loss of aliases, sources, relationships, and warnings when duplicate records were discarded.
- Continued collection after Linux opened-device identity changes/disappearance.
- Windows descriptor offsets, interface-path bounds, and disk/partition response checks.
- Private Linux build RPATH and executable changes during packaging.

## Tested

- macOS Apple Silicon: Release and ASan/UBSan, 8/8 CTest groups.
- Debian x86-64: GCC Release and Clang ASan/UBSan, 11/11 groups each.
- 38 separate merge allocation failures with ownership rollback and zero remaining tracked allocations.
- Windows x86-64 application/C tests cross-compiled with MinGW; portable metadata fixtures run on macOS/Linux.
- Package content/executable equality, native macOS/Linux version/help smoke, documentation/manifest checks, and locally parsed workflow YAML.

Hosted CI must be checked on the owner's actual commit before publication. Native MSVC/runtime acceptance is pending; a workflow definition or cross-build does not close that gate.

## Downloads

| Platform | Asset | Status |
|---|---|---|
| macOS Apple Silicon | disksan-0.1.1-macos-arm64.zip | Native local validation |
| Linux x86-64 | disksan-0.1.1-linux-x86_64.tar.gz | Debian native validation |
| Windows x86-64 | disksan-0.1.1-windows-x86_64.zip | Preview; native acceptance pending |
| Integrity | SHA256SUMS | SHA-256 for final archive bytes |

Extract into a user-owned directory and run bin/disksan or bin/disksan.exe. macOS targets 12.0 or newer; older OS versions were not individually tested. Linux requires system libudev.so.1 and glibc 2.34 or newer. See the distribution guide for native Windows acceptance commands.

## Known Limitations

No erase, sanitize, format, discard, partition modification, or erasure certification is implemented. Snapshots are not atomic and never authorize destructive operations. Physical USB/SATA/NVMe/SCSI/hotplug and complex APFS/RAID/multipath coverage remain limited. Native Windows and comprehensive native API failure injection are open. Packages are unsigned; macOS is not notarized. This is an initial-development 0.x release under Apache License 2.0.
