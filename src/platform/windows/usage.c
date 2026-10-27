/**
 * @file
 * @brief Inspect volume extents and mount paths through zero-access metadata handles only.
 */
#include "disksan/target.h"
#include "metadata.h"
#include <windows.h>
#include <winioctl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/** @brief Bound native volume enumeration and variable-length extent responses. */
#define VOLUME_LIMIT 1024U
/** @brief Maximum extents retained from one read-only query. */
#define EXTENT_LIMIT 64U
/** @brief Accumulate only verified positive volume matches and explicit limitations. */
typedef struct { bool mounted; bool system; bool layered; bool complete; bool changed; } windows_usage;
/** @brief Own bounded, naturally aligned native responses without heap allocation. */
typedef struct { DWORD count; DISK_EXTENT extents[EXTENT_LIMIT]; } extent_buffer;
_Static_assert(offsetof(extent_buffer, extents) == offsetof(VOLUME_DISK_EXTENTS, Extents), "Native extent layout mismatch");

/** @brief Accept a disk number only from the adapter's validated PhysicalDrive alias. */
static bool disk_number(const ds_device *device, DWORD *number)
{
    const char *path = device->role && !strcmp(device->role, "partition") ? device->parent.value : device->path.value;
    const char prefix[] = "\\\\.\\PhysicalDrive";
    uint64_t value;
    if (!path || strncmp(path, prefix, sizeof(prefix) - 1U) ||
        !ds_parse_u64(path + sizeof(prefix) - 1U, &value) || value > UINT32_MAX) return false;
    *number = (DWORD)value;
    return true;
}
/** @brief Resolve the shared system Windows directory, including Terminal Services, without assuming boot completeness. */
static bool system_volume(WCHAR *volume, DWORD capacity)
{
    WCHAR directory[DS_TEXT_LIMIT], root[DS_TEXT_LIMIT];
    memset(directory, 0xff, sizeof(directory)); memset(root, 0xff, sizeof(root));
    UINT count = GetSystemWindowsDirectoryW(directory, (UINT)(sizeof(directory) / sizeof(directory[0])));
    return count > 0U && count < sizeof(directory) / sizeof(directory[0]) &&
           directory[count] == L'\0' && ds_windows_wide_status(directory, count + 1U) == DS_WIDE_OK &&
           GetVolumePathNameW(directory, root, (DWORD)(sizeof(root) / sizeof(root[0]))) &&
           ds_windows_wide_status(root, sizeof(root) / sizeof(root[0])) == DS_WIDE_OK &&
           GetVolumeNameForVolumeMountPointW(root, volume, capacity) &&
           ds_windows_wide_status(volume, capacity) == DS_WIDE_OK;
}
/** @brief Validate a volume GUID path before removing the required enumeration trailing slash. */
static bool volume_name(WCHAR *name, size_t capacity)
{
    const WCHAR *end = wmemchr(name, L'\0', capacity);
    if (ds_windows_wide_status(name, capacity) != DS_WIDE_OK || !end ||
        (size_t)(end - name) < 14U || wcsncmp(name, L"\\\\?\\Volume{", 11) || end[-2] != L'}' || end[-1] != L'\\') return false;
    return true;
}
/** @brief Query one bounded volume extent buffer and match observed physical disk numbers. */
static bool query_extents(HANDLE handle, extent_buffer *buffer)
{
    memset(buffer, 0, sizeof(*buffer)); DWORD returned = 0;
    BOOL queried = DeviceIoControl(handle, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0,
                                   buffer, (DWORD)sizeof(*buffer), &returned, NULL);
    size_t offset = offsetof(extent_buffer, extents);
    if (!queried || returned < offset || returned > sizeof(*buffer) || !buffer->count ||
        buffer->count > EXTENT_LIMIT || buffer->count > (returned - offset) / sizeof(DISK_EXTENT)) return false;
    for (DWORD i = 0; i < buffer->count; ++i) {
        const DISK_EXTENT *extent = &buffer->extents[i];
        if (extent->StartingOffset.QuadPart < 0 || extent->ExtentLength.QuadPart <= 0 ||
            extent->ExtentLength.QuadPart > INT64_MAX - extent->StartingOffset.QuadPart) return false;
    }
    return true;
}
/** @brief Re-query each metadata handle and reject observable extent changes without claiming atomicity. */
static bool matches_volume(const WCHAR *name, DWORD target, windows_usage *usage)
{
    WCHAR handle_name[1024]; size_t length = wcslen(name);
    if (length >= sizeof(handle_name) / sizeof(handle_name[0])) { usage->complete = false; return false; }
    memcpy(handle_name, name, (length + 1U) * sizeof(WCHAR)); handle_name[length - 1U] = L'\0';
    HANDLE handle = CreateFileW(handle_name, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL, OPEN_EXISTING, 0, NULL);
    if (handle == INVALID_HANDLE_VALUE) { usage->complete = false; return false; }
    extent_buffer first = {0}, last = {0};
    bool queried = query_extents(handle, &first);
    bool rechecked = queried && query_extents(handle, &last);
    (void)CloseHandle(handle);
    if (!queried) { usage->complete = false; return false; }
    if (!rechecked || first.count != last.count) { usage->changed = true; return false; }
    bool match = false;
    for (DWORD i = 0; i < first.count; ++i) {
        const DISK_EXTENT *extent = &first.extents[i], *other = &last.extents[i];
        if (extent->DiskNumber != other->DiskNumber || extent->StartingOffset.QuadPart != other->StartingOffset.QuadPart ||
            extent->ExtentLength.QuadPart != other->ExtentLength.QuadPart) { usage->changed = true; return false; }
        match |= extent->DiskNumber == target;
    }
    if (match && first.count > 1U) usage->layered = true;
    return match;
}
/** @brief Check a bounded MULTI_SZ of mounted paths, rejecting truncation and missing terminators. */
static bool mounted_paths(const WCHAR *volume, windows_usage *usage)
{
    WCHAR paths[DS_TEXT_LIMIT]; memset(paths, 0xff, sizeof(paths)); DWORD returned = 0;
    if (!GetVolumePathNamesForVolumeNameW(volume, paths, (DWORD)(sizeof(paths) / sizeof(paths[0])), &returned) ||
        returned > sizeof(paths) / sizeof(paths[0]) || !ds_windows_multisz_valid(paths, returned)) {
        usage->complete = false; return false;
    }
    return paths[0] != L'\0';
}
/** @brief Enumerate metadata-only volume relationships and report unobserved boot/swap/storage scope. */
int ds_collect_usage(const ds_device *device, ds_assessment *result, char *error, size_t size)
{
    windows_usage usage = {false, false, false, true, false}; DWORD target = 0;
    if (disk_number(device, &target)) {
        WCHAR system[1024] = {0}, volume[1024] = {0};
        bool system_known = system_volume(system, (DWORD)(sizeof(system) / sizeof(system[0])));
        if (!system_known) usage.complete = false;
        HANDLE search = FindFirstVolumeW(volume, (DWORD)(sizeof(volume) / sizeof(volume[0])));
        if (search == INVALID_HANDLE_VALUE) usage.complete = false;
        else {
            unsigned count = 0;
            for (;;) {
                if (++count > VOLUME_LIMIT || !volume_name(volume, sizeof(volume) / sizeof(volume[0]))) { usage.complete = false; break; }
                if (matches_volume(volume, target, &usage)) {
                    usage.mounted |= mounted_paths(volume, &usage);
                    usage.system |= system_known && !_wcsicmp(system, volume);
                }
                if (usage.changed) break;
                memset(volume, 0, sizeof(volume));
                if (!FindNextVolumeW(search, volume, (DWORD)(sizeof(volume) / sizeof(volume[0])))) {
                    if (GetLastError() != ERROR_NO_MORE_FILES) usage.complete = false;
                    break;
                }
            }
            (void)FindVolumeClose(search);
        }
    } else usage.complete = false;
    if (usage.changed) { (void)snprintf(error, size, "Volume extents changed or became unavailable during inspection."); return 3; }
/** @brief Never silently drop a native observation or its limitation. */
#define OBS(check, state, source, detail) do { if (ds_assessment_observe(result, check, state, source, detail)) { (void)snprintf(error, size, "Cannot retain volume evidence."); return 1; } } while (0)
    OBS(DS_CHECK_MOUNTED, usage.mounted ? DS_TRUE : DS_UNKNOWN, "windows:volume_extents+mount_paths", "Volume disk extents match the selected disk number; mounted paths establish observed consumers, not exclusive access.");
    OBS(DS_CHECK_SYSTEM, usage.system ? DS_TRUE : DS_UNKNOWN, "windows:SystemWindowsDirectory_volume", "The shared system Windows directory's volume is matched when observable; EFI, recovery, alternate boot and paging dependencies remain incomplete.");
    OBS(DS_CHECK_SWAP, DS_UNKNOWN, "windows:paging_scope", "Active pagefile and hibernation backing were not independently established.");
    OBS(DS_CHECK_SLAVES, usage.layered ? DS_TRUE : DS_UNKNOWN, "windows:volume_extents", "Multiple extents are observed when reported; Storage Spaces, dynamic disks and hidden controller backing remain incomplete.");
    OBS(DS_CHECK_TOPOLOGY, DS_UNKNOWN, "windows:storage_scope", "PhysicalDrive numbering and volume extents do not prove complete storage-layer or physical-device topology.");
    OBS(DS_CHECK_PHYSICAL_SCOPE, DS_UNKNOWN, "windows:controller_scope", "A disk number does not establish controller, namespace or firmware operation scope.");
    OBS(DS_CHECK_USAGE, DS_UNKNOWN, "windows:usage_scope", "Open handles, pagefiles and unenumerated consumers cannot be excluded by volume metadata.");
    OBS(DS_CHECK_HOST_SCOPE, DS_UNKNOWN, "windows:visibility_scope", "Visible volume metadata is non-atomic and subject to access restrictions; no privilege escalation is attempted.");
    OBS(DS_CHECK_COLLECTION, usage.complete ? DS_TRUE : DS_UNKNOWN, "windows:volume_collection", "Denied queries, missing identifiers, buffer limits or enumeration changes remain unknown rather than unsupported or unused.");
#undef OBS
    return 0;
}
