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

Linux opens block nodes with `O_RDONLY | O_CLOEXEC | O_NONBLOCK | O_NOFOLLOW`. Its discovery ioctl allowlist contains only `BLKGETSIZE64`, `BLKSSZGET`, and `BLKPBSZGET`. Permission failures retain available sysfs data and warnings.

macOS reads registry and Disk Arbitration descriptions without opening device data. Windows requests `GENERIC_READ`, falling back to zero access after permission denial. `FILE_SHARE_WRITE` allows sharing with existing OS users; it does not grant this application write access. `IOCTL_DISK_IS_WRITABLE` is a status query and performs no write.

## Extension boundary

Future sanitization needs separate protocol, permission, target-revalidation, policy, and completion-verification layers. Cross-platform inventory does not imply identical destructive capabilities on each operating system. Versions 0.2.0 and 0.3.0 contain no erase backend.

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

## Assessment pipeline in 0.2.0

`src/core/target.c` orchestrates the native selection/usage/reselection window and
is linked with the selected platform adapter to avoid a static-library dependency
cycle. `target_compare.c` is pure, comparing known identity, generation, state and
relationships. The original selector is reused at the closing observation.

`usage.c` builds a bounded connected component over explicit parent/holder/slave
links. Reverse links and shared-parent siblings intentionally overapproximate
possible dependencies. At most 1,024 records participate; ambiguity or limits fail
explicitly. Unknown links are never treated as a complete physical graph.

The Linux adapter maps namespace-local mount IDs, stat-only swap backing and
observed loop filenames into that component, then checks a second graph snapshot.
The macOS adapter uses copied Disk Arbitration volume URLs, cached `getfsstat`
metadata and stat-only node IDs, bracketed by registry snapshots. Windows enumerates
volume GUIDs, queries at most 64 disk extents twice per zero-access handle and
reads bounded mount paths. It cannot establish a durable Windows generation.

`assessment.c` owns fixed-size evidence and derives policy with stable finding
codes. `capabilities.c` makes no native query; six methods retain independently
unknown support/access/applicability/passthrough. `assessment_output.c` renders the
separate schema-2 envelope and method provenance. Old inventory fields use the
same device renderer. No snapshot, token or handle authorizes a future operation.

See [the query audit](READ_ONLY_AUDIT_0.2.0.md) for exact native calls, bounds,
fall-back behavior and test coverage; [the schema](DEVICE_MODEL.md) defines output.

## Explicit protocol pipeline in 0.3.0

`target.c` chooses exactly one capability provider inside the existing selected
target window. `capabilities.c` remains the default metadata-only provider.
`probe.c` invokes the closed native query only after explicit CLI opt-in, validates
the receipt, and emits exactly 24 method/dimension observations. Unknown default
placeholders are never mixed into valid protocol evidence. Overflow or malformed
internal receipts poison the result and prevent success output.

`protocol.c` in the portable core decodes a fixed 4096-byte reply using endian-safe
byte reads and explicit version/field guards. It retains only version, controller
ID, maximum NSID and SANICAP bits. Three NVMe support dimensions may be established;
all remaining dimensions retain their independent evidence state.

The Linux adapter separates `protocol_mapping.c` (fresh direct udev ancestry and
node identity), `protocol_query.c` (one zeroed constant request), and `protocol.c`
(descriptor ownership, pre/post mapping checks, status handling and decoding).
It opens the observed controller character node once with read-only flags. No
controller node is derived from a namespace name. The final original-selector
resolution remains mandatory and no descriptor escapes the operation.

Other platforms compile `protocol_unavailable.c`, with no native protocol calls.
Windows property and macOS SMART interfaces are candidates for separate future
validation, not enabled providers. The [protocol audit](PROTOCOL_QUERY_AUDIT_0.3.0.md)
records exact calls and limitations; the [schema](DEVICE_MODEL.md) defines the
additive opt-in receipt. All snapshots retain false erase authorization.
