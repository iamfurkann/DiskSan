# Architecture

The CLI dispatches to a native discovery adapter, which populates shared C17 metadata and classification types. The output layer renders those records as JSON or terminal-safe text. CMake selects the adapter for the target operating system. Linux-specific APIs stay outside the portable core, and Windows never fabricates Unix device numbers.

## Modules

| Module | Responsibility |
|---|---|
| `src/main.c` | Parse arguments, invoke discovery, render output, release the snapshot, and return a documented exit status |
| `src/core/device.c` | Own metadata, validate numbers, preserve source precedence, deduplicate by native identity, and sort records |
| `src/core/classification.c` | Distinguish role, protocol, transport, and heuristic media without upgrading weak evidence into certainty |
| `src/platform/linux` | Enumerate block devices with libudev, read bounded sysfs attributes, verify identity, and query three read-only ioctls |
| `src/platform/macos` | Read IOMedia/registry and Disk Arbitration metadata; distinguish APFS roles without opening raw devices |
| `src/platform/windows` | Enumerate disk/partition interfaces with SetupAPI and query bounded storage descriptors through read-only or zero-access handles |
| `src/platform/unsupported.c` | Return an explicit error when no native adapter exists |
| `src/output/output.c` | Encode schema 2 JSON, escape terminal metadata, and propagate output failures |

Production discovery does not shell out to diskutil or lsblk. Those tools are read-only references in integration tests. Windows descriptor offsets and string terminators are checked against the returned buffer. Windows CLI arguments are converted from UTF-16 to UTF-8.

## Ownership and errors

Each record owns its text and string lists. Evidence and classification labels are borrowed strings with record lifetime. Adding a record to an inventory transfers ownership only on success; matching duplicate records are combined in a temporary deep copy before either input is released. A failed merge leaves both inputs intact. Callers clean up partial records on every failure path. Public headers describe these contracts.

Missing metadata remains null with warnings. Total discovery failure is not reported as a successful empty snapshot. Source conflicts preserve the first valid value and add a warning. Serial numbers are not deduplication keys: matching native identity, platform, role, and consistent known Unix numbers are required. A shared number/path with conflicting or missing native identity returns an explicit ambiguity error.

Linux rechecks sysfs dev/diskseq and opened descriptor identity. macOS rechecks registry identity and selector stat information. Windows rechecks the storage device number on the opened handle. These checks do not provide an atomic hotplug snapshot or authorization to erase a device.

## Read-only boundary

Linux opens block nodes with `O_RDONLY | O_CLOEXEC | O_NONBLOCK | O_NOFOLLOW`. Its ioctl allowlist contains only `BLKGETSIZE64`, `BLKSSZGET`, and `BLKPBSZGET`. Permission failures retain available sysfs data and warnings.

macOS reads registry and Disk Arbitration descriptions without opening device data. Windows requests `GENERIC_READ`, falling back to zero access after permission denial. `FILE_SHARE_WRITE` allows sharing with existing OS users; it does not grant this application write access. `IOCTL_DISK_IS_WRITABLE` is a status query and performs no write.

## Extension boundary

Future sanitization needs separate protocol, permission, target-revalidation, policy, and completion-verification layers. Cross-platform inventory does not imply identical destructive capabilities on each operating system. Version 0.1.1 intentionally contains no erase backend.

## Primary references

- [Linux block ioctl definitions](https://github.com/torvalds/linux/blob/master/include/uapi/linux/fs.h)
- [libudev enumeration](https://www.freedesktop.org/software/systemd/man/latest/udev_enumerate_add_match_subsystem.html)
- [Apple Disk Arbitration](https://developer.apple.com/library/archive/documentation/DriversKernelHardware/Conceptual/DiskArbitrationProgGuide/Introduction/Introduction.html); installed IOMedia, IOBSD, and storage-characteristics SDK headers also inform the adapter
- [Windows storage property queries](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_storage_query_property)
- [Windows storage descriptor layout](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-storage_device_descriptor)
- [Windows writability status query](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_disk_is_writable)

## Reconciliation and partial snapshots

`ds_inventory_add` returns 0 after insertion/merge, -1 on allocation failure, or -2 for ambiguous identity. Linux/macOS/Windows adapters convert ambiguity into a diagnostic and nonzero scan status (invalid target where applicable). No successful empty JSON is emitted for total failure. Same serial/controller with different native IDs does not merge namespaces or devices. Missing Unix numbers on Windows are never treated as 0:0 evidence. One missing number can be enriched only when the native ID and role agree, with `identity_partially_verified`.

The first normalized, validated field remains primary according to adapter source order. Additional sources/values enter a bounded evidence array; contradictory values add `metadata_conflict`. Alias paths are preserved as observed paths, not permanent identities. Duplicate classification disagreement reduces transport/protocol/media to unknown and warns. Role disagreement rejects the merge. This conservative policy may fail a multipath scan rather than present a false unique target.

Linux collection is separated from enumeration in `device_info.c` with a narrow private seam for deterministic fixtures. `diskseq` is checked before/after collection and included in identity when valid. Without it, syspath/dev_t reuse is not fully detectable and `identity_generation_unavailable` remains visible. A probe that observes replacement/disappearance is dropped, with a scan warning; permission denial preserves valid sysfs evidence. macOS registry references and Windows handles retain their native cleanup paths; comprehensive native API allocation/handle injection remains open.

Windows metadata helpers validate buffer bounds independently of Windows headers. SetupAPI detail paths require a bounded terminator and valid surrogate pairs before conversion, while allocation failure remains fatal. Storage-number type/partition fields are checked before creating a PhysicalDrive alias. Queries remain the existing read-only allowlist. See Microsoft's [interface-detail contract](https://learn.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetdeviceinterfacedetailw) and [storage device numbers](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-storage_device_number).
