# Roadmap

DiskSan targets Linux, macOS, and Windows with one source tree and version number, delivered as separate native packages. Current validation prioritizes macOS and Linux. Native Windows acceptance is deferred to the project owner's Windows device.

## Current candidate

- Linux: live Debian x86-64 inventory, by-id/partition selection, ioctl/libudev fault injection, and sanitizer validation.
- macOS: native IOKit/Disk Arbitration inventory, APFS role distinctions, and live Release/sanitizer validation.
- Windows: SetupAPI/storage-query adapter and MinGW cross-build; native execution remains unverified.
- Distribution: separate CPack archives; packaging does not automatically create a public release.
- Maintenance: English repository content, Doxygen briefs, public API contracts, and documentation checks.

## Open work

| Priority | Work |
|---|---|
| P0 - Windows release gate | Native package, CLI, inventory/partition/info, permissions, and JSON acceptance on Windows |
| P1 | Physical USB/SATA/NVMe/SCSI hardware matrix, hotplug, and device-number reuse |
| P1 | Multi-device APFS/RAID/multipath and Windows volume topology |
| P1 | Native MSVC build and additional Windows descriptor/API fault injection |
| P1 | Alternative-value conflict reporting and comprehensive allocation-failure injection |
| Release decisions | Signing/notarization and tagged binary publication |
| P2 | Linux ARM64, Intel/universal macOS, Windows ARM64, and a broader CI matrix |

Before version 0.2.0, define target revalidation and usage policy that never treats a snapshot identity as erase authorization. Future NVMe/ATA/SCSI sanitization work needs separate platform and hardware requirements. No destructive backend is implemented in this candidate.
