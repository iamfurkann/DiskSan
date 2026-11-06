# DiskSan 0.4.0 - Inspection Planning and Desktop Alpha

DiskSan 0.4.0 is a development pre-release for read-only storage inspection. Select a disk and `disksan plan` explains which observations block a hypothetical operation, which facts are missing and which prerequisites remain unimplemented. `disksan simulate` turns the same model into a ten-step preview. Neither command changes disk data.

## Highlights

- A plan brings identity, usage, topology and method evidence into one report. An observed system, mounted, swap or layered dependency remains blocking even when another observation is incomplete.
- Every simulation labels the steps as modeled. Its schema-2 result includes `simulation_result: "non_executable_model_only"`, `execution_available: false` and `erase_authorized: false`. Exit 0 means the report was generated.
- An optional Qt Quick desktop alpha lets you refresh the live inventory, select a device, read the assessment and preview a simulation. It discards stale or malformed responses and tells you when the CLI cannot complete a request. Screenshots from the synthetic fixture are labeled **DEMO DATA**.
- Existing `list`, `info`, `assess` and `capabilities` commands retain their schema-2 device records and exit classes. The Linux NVMe Identify query still requires an explicit `capabilities --probe` request. Desktop, planning and simulation use metadata-only inspection.

## Try the CLI

Extract the archive into a user-owned directory. From the extracted directory on macOS or Linux, run:

```sh
./bin/disksan list --all --json
printf 'Device path from the inventory: '
IFS= read -r device
./bin/disksan plan -- "$device"
./bin/disksan simulate --json -- "$device"
```

Use the path shown in your own current inventory. A plan has an empty `steps` array. A simulation has ten modeled steps. Both describe a non-atomic observation that expires at the end of inspection. On Windows, run `.\bin\disksan.exe` in PowerShell.

## Downloads

| Platform | CLI asset | Validation |
|---|---|---|
| macOS Apple Silicon | `disksan-0.4.0-macos-arm64.zip` | Native arm64 CLI, deployment target macOS 12.0, exercised on macOS 27.2 |
| Linux x86-64 | `disksan-0.4.0-linux-x86_64.tar.gz` | Native CLI on Debian 13, requires system libudev.so.1 and glibc 2.34 or newer |
| Windows x86-64 | `disksan-0.4.0-windows-x86_64.zip` | MinGW-w64 cross-build, PE architecture and DLL imports checked. Native acceptance pending |
| Checksums | `SHA256SUMS` | SHA-256 digests for the three archives |

The CLI packages have no Qt dependency. The desktop alpha is available as a separate source build with `DISKSAN_BUILD_DESKTOP=ON` and a matching CLI. The tested Qt 6.12.0 SDK targets macOS 14.4 or later for the desktop. Windows Qt and other Qt versions remain untested. See the desktop build instructions in the repository at `docs/GUI_ARCHITECTURE_0.4.0.md`.

## Validation

- On macOS arm64, Release and ASan/UBSan each passed 26/26 CTest groups with the desktop enabled. Native Cocoa tests exercised keyboard navigation, accessible control names and live metadata inspection.
- On Debian 13 x86-64, GCC Release and Clang ASan/UBSan each passed 26/26 CLI test groups. Five additional Qt desktop groups passed in each compiler mode, including live read-only inspection through QML on a terminal-only VM. Physical X11/Wayland interaction remains untested.
- Tests exercised 14,206 plan policy and output assertions, deterministic allocation failures and a fixture that fails on any unexpected protocol query. Across 39 legacy fixture invocations, output matched 0.3.0 after normalizing the version.
- The archives were checked against the exact public file list and the tested executable bytes. macOS and Linux exercised the extracted CLI through all six inspection commands. The unsupported-host CLI subtest was skipped on native hosts.

## Scope and limitations

DiskSan 0.4.0 has no erase, format, discard, firmware mutation or storage user-data I/O backend. A simulation cannot authorize an operation, verify a media effect or generate an erasure certificate.

Physical hotplug and controller coverage, complete RAID/APFS/multipath and Windows dependency visibility, native Windows acceptance and end-to-end screen-reader testing remain open. The archives carry no distribution signature or macOS notarization. A deployable desktop package still requires separate Qt license, deployment and clean-machine review.

DiskSan source is licensed under Apache License 2.0. Qt and its third-party dependencies retain their own licenses.
