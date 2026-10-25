# DiskSan 0.1.0 validation report

This historical report is preserved. See [0.1.1 validation](WORK_REPORT_0.1.1.md) for the next change set.

## 1. Environment and initial state

The project started in an empty, dedicated workspace without Git history or a build system. Only DiskSan files were changed. Linux validation used a newly created, isolated VM workspace with separate source, dependencies, tools, temporary files, builds, and evidence directories. No unrelated project was modified.

The validated environments are macOS arm64 / Darwin 27.0.0, AppleClang 21, CMake 4.4.1, Python 3.14.3; and Debian 13.6 x86-64, kernel 6.12.101, GCC 14.2, Clang 19.1, CMake 3.31.6, libudev 257. Missing VM packages were downloaded and extracted into the project dependency directory without global installation or shell-profile changes.

The owner expanded the original Linux target to Linux, macOS, and Windows, prioritized macOS/Linux, and deferred Windows execution to a personal Windows device. The owner subsequently authorized source publication to [iamfurkann/DiskSan](https://github.com/iamfurkann/DiskSan), English-only repository content, and a consistent `@brief` documentation standard. Machine-specific paths and raw inventories are intentionally excluded from this public report.

## 2. Completed scope

| Work package | Result |
|---|---|
| P0-A | C17/CMake, single version source, help/version CLI, native build selection, and tests |
| P0-B | Linux libudev/sysfs/read-only ioctl discovery and live validation; native macOS IOKit/Disk Arbitration discovery |
| P0-C | Separate role/transport/protocol/media, evidence warnings, checked numbers, native identity, and serial-independent deduplication |
| P0-D | Schema 2 JSON, nullable metadata, exact uint64 capacity, safe terminal output, and CLI regressions |
| P0-E | Release/sanitizer/static-analysis evidence, source transfer verification, documentation, and separate platform packages; native Windows acceptance remains open |

Windows SetupAPI/storage queries and UTF-16 CLI conversion were implemented and cross-compiled. This does not constitute Windows runtime validation. Default listings omit partitions, volumes, and snapshots. No destructive command or graphical interface was added.

## 3. Source manifest and documentation

[FILE_MANIFEST.txt](FILE_MANIFEST.txt) lists every maintained source, header, test, tool, configuration, and Markdown file. The tree includes native adapters, portable core/output, test fixtures, a MinGW toolchain, Doxyfile, contributor guidance, and an automated documentation check.

The repository migration rewrote all maintained prose in English and added file/function briefs, public API parameter/return/ownership contracts, and Python module/class/function documentation. Non-ASCII fixture data uses explicit escapes. Production behavior was preserved; the output fixture's language-specific sample was replaced with an equivalent Unicode boundary sample.

Build directories, archives, generated API HTML, raw logs, local snapshots, and machine-specific inventories are excluded from Git. Existing local evidence remains private. The owner-created Apache License 2.0 and initial Git history are preserved. No user source file was deleted.

## 4. Source transfer integrity

An explicit NUL-separated file list is used with `rsync -avR --from0 --files-from=...`; no `--delete` option is used. Every selected source file is independently hashed using Mac `md5 -q` and Linux `md5sum`, then compared before VM builds. The final transfer covers all 38 files in the current source manifest; all Mac/VM hash pairs match.

Negative checks rejected real content mismatches, empty hash output, failed hash commands, and missing remote files, including a filename with spaces, quotes, and a dollar sign. Finder metadata is excluded from source manifests and archives. These hashes establish transfer equality, not artifact authenticity. Binary archives additionally receive SHA-256 checksums.

## 5. Build and test evidence

The original implementation passed macOS Release and ASan/UBSan runs at 4/4 CTest groups, Linux GCC Debug/Release and Clang ASan/UBSan at 6/6 groups, and Windows MinGW Release cross-compilation. The English documentation migration adds a `documentation` gate. Updated macOS Release and ASan/UBSan tests passed 5/5 groups, and Linux GCC Release plus Clang ASan/UBSan passed 7/7 groups. The documentation gate passes for all 38 maintained files, covering 109 C declarations/definitions and 51 Python classes/functions. Negative probes reject missing briefs and incomplete public parameter contracts. Doxygen 1.9.8 generated HTML successfully with warnings treated as errors and an empty diagnostic log. The Windows MinGW Release cross-build also passed after the documentation migration.

Behavioral coverage consists of 233 core assertions / 24 classification fixtures; 16 output tests including 11 UTF-8 boundary inputs; 5 CLI tests with 17 invalid command forms; 171 Linux probe assertions; 40 libudev boundary assertions across 7 scenarios; and 5 native integration tests on each of Linux and macOS. Supported platforms skip the unsupported-host CLI scenario as intended. The recorded Linux integration run did not skip by-id or live-node tests.

Static analysis of 5 Mac and 6 Linux production files completed with no diagnostics. Earlier UTF-8 buffer-boundary and sysfs fread/EOF analysis findings were fixed and regression-tested. MinGW header compatibility was resolved using the read-only DISK_IS_WRITABLE status query. Native Windows's 3 prepared integration tests and MSVC builds remain unexecuted.

## 6. Live inventory findings

Linux exposed 3 default records and 6 with `--all`: fd0, sda, sr0, and three sda partitions. The QEMU ATA disk reported 34,359,738,368 bytes with heuristic HDD classification. The optical device reported 1,073,741,312 bytes and unknown media. Normal-user permission denial retained sysfs fallback metadata and warnings. Capacity/device-number comparisons with sysfs and lsblk passed.

The real Mac exposed 20 records: 1 disk, 3 partitions, 3 APFS containers, 12 volumes, and 1 snapshot; the default list showed 4 records. The main disk reported 251,000,193,024 bytes, heuristic SSD media, and unknown transport/protocol where native evidence was insufficient. Missing physical sector size remained null. Structured diskutil comparisons passed.

NVMe namespace, shared-controller identity, SATA, USB bridge, virtio, loop, dm, md, SCSI optical, and APFS classifications also have synthetic fixtures. This VM does not expose virtio/NVMe devices. Fixtures and virtual ATA devices do not establish physical hardware compatibility.

## 7. Read-only behavior and safety checks

No erase, sanitize, format, discard, or device-data mutation was implemented or performed. Linux opens block nodes only with O_RDONLY, O_CLOEXEC, O_NONBLOCK, and O_NOFOLLOW; the closed ioctl allowlist contains BLKGETSIZE64, BLKSSZGET, and BLKPBSZGET. A live strace audit recorded 6 read-only opens, 3 allowed queries, and zero device writes.

macOS uses registry/description APIs without opening device data. Windows requests read or zero access, never GENERIC_WRITE. FILE_SHARE_WRITE is a sharing flag, not write authorization. Selector checks, identity revalidation, invalid-path rejection, valid symlink resolution, bounded metadata, terminal escaping, and JSON encoding have targeted tests.

Snapshots are not atomic. Native IDs and serials are not erase authorization. Physical hotplug races and every allocation-failure point have not been exhaustively tested.

## 8. Open work

The Windows release gate is native package/CLI/inventory/partition/info/permission/JSON acceptance on a real Windows host. Until then, the Windows artifact is a preview. Physical hardware coverage, hotplug/device-number reuse, richer APFS/RAID/multipath topology, MSVC builds, and more extensive fault injection remain open.

Other architectures, CI expansion, signing/notarization, and tagged binary publication are separate work. Before version 0.2.0, define target revalidation and usage policy independently of snapshot identity. No version 0.2.0 or destructive backend was implemented.

## 9. Repository and distribution

The owner authorized the first source upload to the canonical GitHub repository. That authorization supersedes the initial development document's no-commit/no-push restriction. This source-publication task does not create a tag, binary release, or signing/notarization claim.

Version 0.1.0 comes from CMake PROJECT_VERSION. CPack produces macOS arm64 ZIP, Linux x86_64 tar.gz, and Windows x86_64 ZIP. Archive checks cover expected files, matching documentation, checksums, and executable identity. The Mac package was extracted with executable permissions intact and passed version/list/info. Linux passed those commands from a relocated archive using system libraries without build-directory RPATH or custom LD_LIBRARY_PATH. Windows ZIP CRC and PE/import checks passed; native execution remains pending.

Use the [distribution guide](DISTRIBUTION.md) to regenerate packages and run native acceptance. Generated binaries and local evidence are not source commits. Source publication and the availability of a tagged release are separate states.

## 10. Readiness assessment

The read-only macOS and Linux implementation is a tested 0.1.0 candidate. Windows has a cross-compiled preview awaiting native acceptance. A fully validated, signed, three-platform binary release is not claimed. The source is prepared for publication with English documentation, explicit API contracts, and an automated standard check; remaining runtime and release gates are documented in the roadmap.
