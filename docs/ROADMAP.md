# Roadmap

One C17 source tree targets Linux, macOS, and Windows, with separate platform/architecture downloads. Native validation prioritizes macOS/Linux; Windows remains a preview until the owner's machine and hosted MSVC gates pass.

## 0.1.1 - Reliability & Release Hardening

Transactional identity reconciliation, explicit ambiguity failures, bounded alternative-source evidence, Linux generation/partial-scan fixtures, portable Windows metadata validation, CI definitions, and archive checks are implemented. See the versioned validation report for executed evidence. Hosted CI and native Windows acceptance must be observed, not inferred from workflow/source existence.

## Remaining gates

| Priority | Work |
|---|---|
| P0 - Windows release gate | NATIVE_WINDOWS_ACCEPTANCE_PENDING: MSVC build, real CLI/JSON/inventory/partition/info, ordinary-user permissions, and package acceptance |
| P1 | Physical USB/SATA/NVMe/SCSI, hotplug, real device-number reuse, and bridge evidence |
| P1 | Comprehensive native Windows/IOKit allocation/handle failure injection; current synthetic helpers do not cover all native API paths |
| P1 | Complete multi-device APFS/RAID/multipath and Windows Storage Spaces/dynamic-disk topology |
| Release gates | Hosted CI on the committed revision, maintainer publication, optional signing/notarization |
| P2 | Linux ARM64, Intel/universal macOS, Windows ARM64, and broader runtime compatibility |

## 0.2.0 - Read-only Target Assessment (published baseline)

Target revalidation, usage-policy checks, explicit capability uncertainty and new CLI/JSON commands are implemented. The published baseline's four hosted jobs, including MSVC, passed on its tagged SHA; the actual reference was rechecked in [current M0 evidence](PROGRESS_0.3.0.md). Historical local results remain in [0.2.0 progress](PROGRESS.md). That evidence does not close current-version CI or physical-hardware gates. Snapshots remain non-atomic and never authorize erasure.

## 0.3.0 - Audited Read-only Capability Discovery

Explicit opt-in Linux NVMe Identify Controller, versioned decoding, independent
capability evidence and native fault tests are the current candidate scope.
Windows/macOS/ATA/SCSI protocol paths remain unavailable until individually
audited and validated. See [current progress](PROGRESS_0.3.0.md) and the
[longer-term roadmap](ROADMAP_TO_1.0.0.md).

## Later releases

Any NVMe/ATA/SCSI sanitize implementation needs a separate specification, platform/hardware scope, explicit target authorization, completion verification, and failure/recovery design. No destructive API, preflight write, or erasure certificate is part of 0.2.0 or 0.3.0.
