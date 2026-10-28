# DiskSan

DiskSan is a **read-only disk discovery and target-assessment CLI** written in C17. It shares a portable metadata and classification core across Linux, macOS, and Windows, with a native discovery adapter for each operating system.

Version 0.3.0 adds explicitly requested, audited NVMe controller capability discovery on eligible Linux targets. Default commands remain metadata-only. It inventories devices and reports evidence and uncertainty. It does not erase, sanitize, format, discard, modify partitions, or issue erasure certificates. The current interface runs in a terminal; there is no graphical application yet.

Repository: [iamfurkann/DiskSan](https://github.com/iamfurkann/DiskSan).

## Platform status

| Platform | Native discovery | Explicit protocol provider |
|---|---|---|
| Linux x86-64 | libudev, sysfs, three read-only block ioctls | Fixed NVMe Identify Controller for verified direct PCI namespace/controller mappings; synthetic native tests, physical acceptance pending |
| macOS arm64 | IOKit and Disk Arbitration | Unavailable; no audited protocol provider enabled |
| Windows x86-64 | SetupAPI and storage queries | Unavailable; native MSVC/property-query acceptance gate remains open |

The Windows package is a **preview** until native acceptance tests pass. Each platform has a separate archive with the same version number. See [distribution](docs/DISTRIBUTION.md) for filenames, requirements, and package validation. Source publication does not imply a tagged binary release.

## Assessment and explicit capability discovery

`assess` explains observed mounts, system/swap dependencies, logical layers and unresolved scope. `capabilities` reports six method families with independent support, OS access, applicability and passthrough states. Without `--probe`, all method states remain `unknown`. Only `capabilities <selector> --probe` selects the fixed Linux controller query. Valid SANICAP fields may establish three NVMe support states; OS method access, applicability and passthrough remain independent and unknown. Query denial is unavailable, never hardware unsupported. A successful inspection never authorizes erasure.

Repeated native identities are reconciled without discarding additional metadata, aliases, relationships, or warnings. Conflicting identity/role observations fail explicitly. Bounded supplemental evidence exposes alternate sources while preserving JSON schema 2. Linux generation changes and partial scans have deterministic fault tests.

[CI](docs/CI.md) defines Linux GCC, Linux Clang ASan/UBSan, macOS AppleClang, and Windows MSVC jobs. A checked-in workflow is not evidence of a successful hosted run. See [requirement acceptance](docs/ACCEPTANCE_0.3.0.md), [current progress and evidence](docs/PROGRESS_0.3.0.md) and the [0.3.0 release notes](docs/RELEASE_NOTES_0.3.0.md) for executed checks and remaining gates; [security reporting](SECURITY.md) covers sensitive findings.

## Build and test

Requirements: a C17 compiler, CMake 3.20 or newer, and Python 3.9 or newer for tests. Linux additionally needs pkg-config, libudev development files and Linux UAPI headers including `linux/nvme_ioctl.h`. macOS uses the Xcode Command Line Tools and system IOKit, CoreFoundation, and Disk Arbitration frameworks. Windows targets Visual Studio with C17 support or MinGW-w64. Tagged 0.2.0 MSVC CI passed; that historical run is not validation of the current 0.3.0 working tree.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

On Debian/Ubuntu, missing build dependencies can be installed with:

```sh
sudo apt-get install cmake pkg-config libudev-dev build-essential python3
```

Builds and discovery tests run as a normal user. A macOS application sandbox may restrict native discovery APIs; run the CLI from a normal terminal. Tests never require sudo or modify block-device data.

Visual Studio typically produces `build/Release/disksan.exe`. Single-configuration Unix builds produce `build/disksan`; MinGW builds produce `build/disksan.exe`.

## Usage

```sh
./build/disksan --version
./build/disksan --help
./build/disksan list
./build/disksan list --all --json
# Linux: use a device path shown in the list.
./build/disksan info /dev/sda --json
# macOS:
./build/disksan info /dev/disk0 --json
./build/disksan assess /dev/disk0 --json
./build/disksan capabilities /dev/disk0
./build/disksan assess --help
```

Windows PowerShell, from an extracted package:

```powershell
.\bin\disksan.exe list --json
.\bin\disksan.exe info '\\.\PhysicalDrive0' --json
```

Device paths are examples: select an actual record from your inventory. Windows also accepts enumerated partition interface paths. macOS accepts `/dev/diskN` block nodes and rejects `/dev/rdiskN` raw character nodes.

The default list omits partitions, volumes, and snapshots. `--all` includes every observed role. APFS containers, volumes, and snapshots are distinct records, so the number of records is **not** a physical disk count.

Place `--json` after the subcommand; `info`, `assess` and `capabilities` accept it before or after the selector. `--` ends option parsing. Exit statuses are 0 for success, 1 for system/output failure, 2 for invalid usage, 3 for an invalid or changed target, and 4 for a selector permission error. Exit 0 means the inspection completed; assessment may still be `blocked` or `review_required`. Success may include warnings and incomplete metadata. Fatal diagnostics go to stderr.

For a deliberate protocol check, select a non-system test namespace yourself and run
`disksan capabilities --json --probe <selector>`. Do not copy a boot-disk example
into this command. No environment variable enables probing, and other subcommands
reject `--probe` before discovery. Only one fixed controller Identify request is
allowed; no raw opcode, timeout, length, namespace or ioctl parameter is accepted.
See the [query audit](docs/PROTOCOL_QUERY_AUDIT_0.3.0.md) and
[manual acceptance procedure](docs/TESTING.md#manual-protocol-acceptance).

## Data contract

JSON uses **schema_version 2**, including `platform`, `device_id`, and nullable Unix device numbers. Unknown numeric/text fields are null; unknown classifications use `unknown`. Capacity is an exact uint64 integer. Evidence sources and warnings remain visible. `list`/`info` retain their inventory envelope. New commands use a separate envelope with `device`, `identity`, `observations`, `findings`, `capabilities`, `inspection_status`, and literal `erase_authorized: false`. Explicit probing adds a bounded `probe` receipt; default output omits it. See the [device model](docs/DEVICE_MODEL.md).

USB transport does not prove the underlying ATA or NVMe protocol. Media classifications are heuristics, not physical-hardware guarantees. Native IDs, serial numbers, and aliases do not authorize future erasure operations.

## Development standard

Repository documentation, comments, identifiers, diagnostics, and contributor-facing text are English. Every C file and function has Doxygen `@brief` documentation. Public APIs also describe parameters, ownership, and return values. Python test modules, classes, and functions follow the same brief-description convention.

```sh
python3 tools/check_documentation.py
# Optional HTML API reference, with Doxygen installed:
doxygen Doxyfile
```

See [contributing](CONTRIBUTING.md) and the [coding standard](docs/CODING_STANDARD.md). Generated API documentation, packages, build directories, and local evidence are excluded from source control.

## Limitations

- Current 0.3.0 native Windows/MSVC, physical protocol queries, hotplug and a broad USB/SATA/NVMe/SCSI hardware matrix remain separate acceptance gates. Historical 0.2.0 hosted tests and current cross-compilation do not close them.
- Linux probing requires a known disk generation, direct PCI NVMe ancestry and stable namespace/controller mapping. USB, fabrics, aggregate/multipath heads, observed layered targets and missing mappings are declined. The decoder accepts reviewed NVMe 1.3.0, 1.4.0 and 2.0.0 layouts; other versions remain unknown. Controller support does not establish namespace applicability.
- The Linux ioctl requests exactly 4096 bytes but returns no byte count. Required fields and completion status are checked; truthful firmware content, atomicity and future identity cannot be guaranteed. The 5000 ms driver timeout is not a userspace wall-clock guarantee.
- macOS reports the nearest observed IOMedia parent; complete multi-device APFS/RAID topology is not guaranteed. Unavailable physical sector sizes remain null.
- Windows may expose less metadata to a normal user. Missing values remain null with warnings; media stays unknown without rotational evidence. Volume extents and mount paths are observed when accessible; paging, EFI/recovery and hidden Storage Spaces/dynamic-disk backing remain unknown.
- Linux mount evidence covers the current namespace only. Loop filenames mapped by stat cannot prove the kernel-held backing inode; hidden or layered filesystem backing remains unknown.
- Initial packages target macOS arm64, Linux x86-64, and Windows x86-64. Other architectures are unverified.
- The project uses the [Apache License 2.0](LICENSE). Signing, notarization, and tagged binary publication are separate release decisions.

[Architecture](docs/ARCHITECTURE.md) | [Testing](docs/TESTING.md) | [Roadmap](docs/ROADMAP.md) | [Changelog](CHANGELOG.md) | [Validation report](docs/WORK_REPORT.md)
