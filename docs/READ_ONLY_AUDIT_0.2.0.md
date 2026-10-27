# Read-only boundary audit for 0.2.0

This source review covers the implemented assessment path. It is not a claim of
physical controller validation or a complete runtime syscall trace. Test evidence
and source-snapshot scope are recorded in [PROGRESS.md](PROGRESS.md).

## Portable boundary and existing discovery

`assessment.c`, `target_compare.c`, `usage.c`, `capabilities.c` and renderers use
ordinary memory/streams only. The assessment model uses bounded inline arrays;
invalid/oversized evidence poisons evaluation. `target.c` repeats native discovery
with the original selector around usage and capability collection. Changed or
lost known identity/state returns a nonzero exit with no success assessment.
Neither a handle nor a token is returned for later operations. ABA replacement,
metadata changes outside the window and non-atomic queries remain explicit limits.

The inventory allowlists remain unchanged: Linux block nodes are opened read-only
and receive only BLKGETSIZE64, BLKSSZGET and BLKPBSZGET; macOS uses registry and Disk
Arbitration descriptions; Windows uses the existing storage-property, number,
length, geometry, alignment and writability-status queries. No write access is
added. See [ARCHITECTURE.md](ARCHITECTURE.md) and existing boundary tests.

The capability provider issues **zero identify/log or protocol commands**. All six
methods retain unknown support, OS access, applicability and passthrough. Native
metadata access is never promoted into protocol permission or method support.
Synthetic supported/unsupported/unavailable fixtures test semantics only.

## Linux usage allowlist

| Operation | Access, bounds and purpose | Failure behavior and tests |
|---|---|---|
| `open`, `fstat`, `fstatfs`, `fdopen`, `fgetc`, `ferror`, `fclose` on `/proc/self/mountinfo` and `/proc/swaps` | O_RDONLY, O_CLOEXEC, O_NOFOLLOW, O_NONBLOCK; require regular file on PROC_SUPER_MAGIC before reading. Maximum 16 MiB/source, 16,384 bytes/record. | Inaccessible/wrong filesystem/type or malformed data remains incomplete; allocation failure is fatal. Link wrappers assert path/flags, substitution rejection and closure. |
| Same metadata operations on `/sys/devices/virtual/block/loopN/loop/backing_file` | Numeric canonical loop suffix; require regular SYSFS_MAGIC attribute. At most 4,096 filename bytes plus final newline. Loop name only selects a query, not capability classification. | Missing, oversized or inaccessible attribute remains unknown. Repeated name changes return exit 3. Synthetic loop fixtures cover permission, size, filesystem and change paths. |
| `stat` of proc swap entries and loop backing names | Metadata only; never open/read backing files or device content. Validate file/block type; compare repeated dev/inode/rdev/type. Map st_dev (file) or st_rdev (block) into bounded observed graph. | Replacement fails or leaves incomplete evidence. Deleted/hidden/layered backing cannot establish absence. Fault fixtures use synthetic stat structures only. |
| Repeated `ds_discover` and observed graph comparison | Existing libudev/sysfs/block metadata; max 1,024 graph records. Compare related native identities and relationships after usage observation. | Changed membership/generation/backing or ambiguity fails. Existing allocation tests plus target/graph/native usage fixtures cover partial cleanup and conservative results. |

Proc/sysfs reads query kernel metadata only. O_NONBLOCK prevents a substituted
FIFO from blocking before descriptor validation. Read-only opening alone would
not establish this boundary; file type and filesystem are checked before bytes
are consumed. No mount changes, loop setup/control ioctl, discard, lock probe,
block data read or privilege escalation exists.

Loop filename mapping is evidence about the current name's stat metadata, not
proof of the inode held by the loop driver. A renamed/deleted/replaced file,
hidden namespace, multi-device filesystem, or unchanged-path ABA event can remain
unresolved. The adapter retains unknown topology/usage scope. It does not infer
idle media from an empty mount or swap list. Shared parent links conservatively
include siblings, which may produce broader blocking findings.

References: [proc metadata](https://docs.kernel.org/filesystems/proc.html),
[mountinfo](https://man7.org/linux/man-pages/man5/proc_pid_mountinfo.5.html),
[swaps](https://man7.org/linux/man-pages/man5/proc_swaps.5.html),
[stat](https://man7.org/linux/man-pages/man2/stat.2.html),
[statfs filesystem types](https://man7.org/linux/man-pages/man2/statfs.2.html),
[loop read-only ABI](https://www.kernel.org/doc/html/latest/admin-guide/abi-testing-files.html#abi-file-testing-sysfs-block-loop).
Kernel loop backing-file output is raw filename bytes plus newline; it is not the
proc octal escape format. The collector preserves that distinction.

## macOS usage allowlist

| Operation | Access, bounds and purpose | Failure behavior and tests |
|---|---|---|
| `DASessionCreate`, `DADiskCreateFromBSDName`, `DADiskCopyDescription` | Copy descriptions for observed component members, max 1,024. No claim, mount, unmount, raw-device open or mutation API. | Null session/disk/description leaves incomplete evidence; release every copied reference. Synthetic native fault object verifies cleanup. |
| `CFDictionaryGetValue(kDADiskDescriptionVolumePathKey)`, `CFGetTypeID`, `CFURLGetTypeID`, `CFURLGetFileSystemRepresentation`, `CFRelease` | Type-check borrowed URL, copy at most 4,096 path bytes plus terminator; compare root/system mount names. | Wrong type or conversion failure remains unknown. Real in-memory CF fixtures exercise type/URL/refcount paths. |
| `getfsstat` with `MNT_NOWAIT` | Cached metadata count, bounded calloc of count+16 records, max 1,024; validate returned count and fixed string terminators. | Changed/full/error table remains incomplete; allocation failure is fatal. Fault fixtures cover allocation, counts, strings and cleanup. |
| `stat` on reported `/dev/` mount sources | Metadata only; require block node and match st_rdev against observed registry component. | Failure remains unknown; fake node metadata in fault tests, live root/alias check on Mac. |

Installed SDK declarations define the CFURL value type. See Apple's
[Disk Arbitration overview](https://developer.apple.com/library/archive/documentation/DriversKernelHardware/Conceptual/DiskArbitrationProgGuide/Introduction/Introduction.html),
[volume path property](https://developer.apple.com/documentation/diskarbitration/kdadiskdescriptionvolumepathkey)
and [getfsstat contract](https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/getfsstat.2.html).
No new runtime library is required: existing system frameworks provide these APIs.
Native Mac Release and sanitizer tests passed, but full discovery API injection,
APFS multi-device/Fusion/RAID and physical hotplug remain outside that evidence.

## Windows usage allowlist

| Operation | Access, bounds and purpose | Failure behavior and tests |
|---|---|---|
| `FindFirstVolumeW`, `FindNextVolumeW`, `FindVolumeClose` | At most 1,024 volume GUID paths, 1,024 UTF-16 units/path; validate type/terminators before use and close enumeration handle. | Enumeration failure or bounds remain incomplete. Synthetic wrappers assert cleanup and malformed enumeration behavior; cross-build PASS, native execution NOT_RUN. |
| `CreateFileW`, `CloseHandle` on volume GUID path without final slash | Desired access **0**, OPEN_EXISTING, no write access. SHARE_READ/WRITE/DELETE permit other users; they do not authorize application writes. | Denial leaves unknown evidence; no elevation or alternative access escalation. |
| `DeviceIoControl(IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS)` | No input buffer; bounded aligned stack storage for max 64 extents. Validate returned count/bytes and positive non-overflow ranges. Query twice on the same handle. | Invalid first response is unknown; changed/lost known second response fails with exit 3. Oversized responses are not silently accepted; prepared fault scenarios cover counts, ranges and closing-query changes. |
| `GetVolumePathNamesForVolumeNameW` | Max 4,096 UTF-16 units; validate every MULTI_SZ string/surrogate and final empty entry. | Missing/truncated/malformed names remain incomplete. Portable metadata fixtures cover UTF-16 and MULTI_SZ boundaries. |
| `GetSystemWindowsDirectoryW`, `GetVolumePathNameW`, `GetVolumeNameForVolumeMountPointW` | Bounded, validated strings map the shared system Windows directory to a volume name, including Terminal Services. | Failure remains unknown. Does not claim EFI/recovery, pagefile, hibernation or all boot dependencies. |

The sole new control code returns extent metadata, not storage content. This
adapter has no heap allocations. It cannot prove complete Storage Spaces,
dynamic-disk, namespace or physical topology. Per-handle extent rechecks and the
closing selected-identity query do not make the entire enumeration atomic.

References: [extent query](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_volume_get_volume_disk_extents),
[extent response](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-volume_disk_extents),
[volume enumeration](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findfirstvolumew),
[zero-access metadata handles](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew),
[volume paths](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getvolumepathnamesforvolumenamew),
[root volume path](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getvolumepathnamew),
[shared Windows directory](https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getsystemwindowsdirectoryw),
[volume GUID from mount point](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getvolumenameforvolumemountpointw),
[continued enumeration](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findnextvolumew),
[enumeration cleanup](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findvolumeclose),
[handle cleanup](https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle).
The 16-scenario Windows all-query fault executable cross-builds; its execution and native acceptance remain NOT_RUN;
MinGW compilation and portable buffer tests do not close that gate.

## Review conclusion and remaining gates

No executable erase/sanitize/format/discard/mount modification, firmware command,
block data read/write or elevation path was added. Metadata-query and allocation
fault tests use ordinary temporary files or synthetic memory. Runtime tests query
only accessible native metadata. Root/system dependencies are explicitly blocking;
unknown scope is retained rather than equated to safety.

The final source-pattern/call review and regression evidence are recorded separately in
[the acceptance matrix](ACCEPTANCE_0.2.0.md); Windows native and hardware gates remain open. This
source audit does not substitute for those execution results or claim a full
syscall trace. No release, tag, commit or publication is authorized by this audit.
