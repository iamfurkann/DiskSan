# Changelog

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
