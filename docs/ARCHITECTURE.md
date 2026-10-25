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

Each record owns its text and string lists. Evidence and classification labels are borrowed strings with record lifetime. Adding a record to an inventory transfers ownership only on success; duplicate records are consumed and released. Callers clean up partial records on every failure path. Public headers describe these contracts.

Missing metadata remains null with warnings. Total discovery failure is not reported as a successful empty snapshot. Source conflicts preserve the first valid value and add a warning. Serial numbers are not deduplication keys: native identity or a valid same-platform Unix device number is required.

Linux rechecks sysfs dev/diskseq and opened descriptor identity. macOS rechecks registry identity and selector stat information. Windows rechecks the storage device number on the opened handle. These checks do not provide an atomic hotplug snapshot or authorization to erase a device.

## Read-only boundary

Linux opens block nodes with `O_RDONLY | O_CLOEXEC | O_NONBLOCK | O_NOFOLLOW`. Its ioctl allowlist contains only `BLKGETSIZE64`, `BLKSSZGET`, and `BLKPBSZGET`. Permission failures retain available sysfs data and warnings.

macOS reads registry and Disk Arbitration descriptions without opening device data. Windows requests `GENERIC_READ`, falling back to zero access after permission denial. `FILE_SHARE_WRITE` allows sharing with existing OS users; it does not grant this application write access. `IOCTL_DISK_IS_WRITABLE` is a status query and performs no write.

## Extension boundary

Future sanitization needs separate protocol, permission, target-revalidation, policy, and completion-verification layers. Cross-platform inventory does not imply identical destructive capabilities on each operating system. Version 0.1.0 intentionally contains no erase backend.

## Primary references

- [Linux block ioctl definitions](https://github.com/torvalds/linux/blob/master/include/uapi/linux/fs.h)
- [libudev enumeration](https://www.freedesktop.org/software/systemd/man/latest/udev_enumerate_add_match_subsystem.html)
- [Apple Disk Arbitration](https://developer.apple.com/library/archive/documentation/DriversKernelHardware/Conceptual/DiskArbitrationProgGuide/Introduction/Introduction.html); installed IOMedia, IOBSD, and storage-characteristics SDK headers also inform the adapter
- [Windows storage property queries](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_storage_query_property)
- [Windows storage descriptor layout](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-storage_device_descriptor)
- [Windows writability status query](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_disk_is_writable)
