# Testing

## Build and run

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake -S . -B build-sanitize -DCMAKE_BUILD_TYPE=Debug -DDISKSAN_SANITIZERS=ON
cmake --build build-sanitize
ctest --test-dir build-sanitize --output-on-failure
python3 tools/check_documentation.py
```

GCC/Clang builds enable Wall, Wextra, Wpedantic, Wconversion, Wshadow, and Werror by default. MSVC uses W4/WX, but a native MSVC build remains unverified. Windows sanitizer support is not part of the current validation evidence.

## Test groups

| CTest group | Coverage |
|---|---|
| documentation | Source manifest, ASCII English-source policy, file/function briefs, public API parameter contracts, and Python docstrings |
| core | 233 assertions, 24 classification fixtures: overflow, source conflicts, duplicate serials, native identity, and absent dev_t |
| output_contract | 16 tests: schema 2, UTF-8, exact uint64, null, terminal safety, and 11 Unicode boundary inputs |
| cli | 5 tests, including 17 invalid command forms and output failure; unsupported-host coverage skips on supported platforms |
| block_info_errors - Linux | 171 assertions for access denial, device disappearance, changed type/number, unsupported ioctls, and read-only flags |
| discovery_errors - Linux | 40 assertions across 7 scenarios: libudev initialization/allocation/filter/scan failures, empty snapshots, and disappeared devices |
| linux_integration | 5 tests for live inventory/info/by-id/partitions, invalid targets, and sysfs/lsblk comparison |
| macos_integration | 5 tests for diskutil plist comparison, native identity/flags, filtering, info/symlinks, and invalid selectors |
| windows_integration | 3 prepared native tests for identity/info, partition filtering, and ordinary-file rejection; not yet executed |

The documentation gate increases the native totals from the original 4 macOS / 6 Linux groups to **5 macOS / 7 Linux groups**. See the [validation report](WORK_REPORT.md) for recorded results. CTest group counts are distinct from individual assertions or Python test cases.

## Platform evidence

macOS arm64 / Darwin 27, AppleClang 21: Release and ASan/UBSan checks ran on a real Mac. The observed inventory had 20 records: 1 disk, 3 partitions, 3 APFS containers, 12 volumes, and 1 snapshot; the default list had 4 records. Counts and roles are compared with structured diskutil plist data. The product never invokes diskutil and the Mac adapter never opens raw block data.

Linux x86-64 / Debian 13.6, kernel 6.12.101: GCC 14.2, Clang 19.1, CMake 3.31.6, libudev 257. Dependencies were extracted into an isolated project directory without modifying system packages. The VM exposed fd0, sda, sr0, and three sda partitions. A normal user saw permission denial on sda and retained sysfs fallback metadata. A recorded strace audit observed 6 O_RDONLY device opens, 3 allowed query ioctls, and 0 device writes.

Windows x86-64: MinGW-w64 Release cross-compilation succeeded. The PE import table contains KERNEL32.dll, msvcrt.dll, and SETUPAPI.dll, with no additional MinGW runtime DLL. Cross-compilation is not native execution evidence. Windows acceptance remains deferred to the owner's Windows device; commands are in [distribution](DISTRIBUTION.md).

## Boundaries and reproducibility

Physical USB/SATA/NVMe/SCSI hardware, hotplug, multi-device APFS/RAID/multipath, and every allocation point have not been tested. Fixtures are not physical hardware validation. This Linux VM does not expose virtio/NVMe devices, so those classifications are fixture-tested only.

Tests never invoke sudo, mount, format, or erasure. They may create ordinary temporary fixture files; TMPDIR can point into an isolated workspace. Integration tests report missing device/by-id prerequisites as skips. The recorded Linux run completed all five integration tests without such skips.

Mac sources are transferred using an explicit file manifest and independently compared with Mac md5 / Linux md5sum. Negative checks reject mismatched contents, empty hashes, failed hash commands, and missing files. Hashes validate transfer equality, not authenticity. Package artifacts additionally receive SHA-256 checksums. Local logs, machine-specific paths, and raw inventories remain outside the public source tree.

Optional API rendering uses `doxygen Doxyfile`. The Python documentation gate requires no Doxygen installation and checks the project's conventional C declarations plus Python AST docstrings; it is not a general-purpose C parser or natural-language detector. English wording and comment accuracy also require review.
