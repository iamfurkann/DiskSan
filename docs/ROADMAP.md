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

## 0.2.0 - Read-only Target Assessment (unpublished candidate)

Target revalidation, usage-policy checks, explicit capability uncertainty and new CLI/JSON commands are implemented. Native Linux/macOS and synthetic checks have passing evidence; the final acceptance matrix and external Windows/CI/hardware gates are tracked in [progress](PROGRESS.md). Snapshots remain non-atomic and never authorize erasure. Define identity freshness and policy failure behavior before considering protocol operations.

## 0.3.x and later

Any NVMe/ATA/SCSI sanitize implementation needs a separate specification, platform/hardware scope, explicit target authorization, completion verification, and failure/recovery design. No destructive API, preflight write, or erasure certificate is part of 0.2.0.
