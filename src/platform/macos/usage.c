/**
 * @file
 * @brief Observe mount metadata and Disk Arbitration descriptions without opening device data.
 */
#include "disksan/target.h"
#include "disksan/discovery.h"
#include "disksan/usage.h"
#include <DiskArbitration/DiskArbitration.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>

/** @brief Accumulate positive mount/system evidence and incomplete native observation status. */
typedef struct { bool mounted; bool system; bool complete; } mac_usage;
/** @brief Recognize root and known system-volume mount paths without claiming all boot dependencies. */
static bool system_path(const char *path)
{
    return !strcmp(path, "/") || !strcmp(path, "/System/Volumes") ||
           !strncmp(path, "/System/Volumes/", 16) || !strcmp(path, "/private/var");
}
/** @brief Read copied Disk Arbitration volume paths for records already in the observed component. */
static void arbitration_mounts(const ds_inventory *inventory, const ds_usage_graph *graph, mac_usage *usage)
{
    DASessionRef session = DASessionCreate(kCFAllocatorDefault);
    if (!session) { usage->complete = false; return; }
    for (size_t i = 0; i < graph->count; ++i) if (graph->related[i]) {
        const char *name = inventory->items[i].path.value;
        DADiskRef disk = name ? DADiskCreateFromBSDName(kCFAllocatorDefault, session, name) : NULL;
        CFDictionaryRef description = disk ? DADiskCopyDescription(disk) : NULL;
        if (!description) usage->complete = false;
        else {
            CFTypeRef value = CFDictionaryGetValue(description, kDADiskDescriptionVolumePathKey);
            if (value) {
                UInt8 path[DS_TEXT_LIMIT + 1U];
                if (CFGetTypeID(value) == CFURLGetTypeID() && CFURLGetFileSystemRepresentation(value, true, path, (CFIndex)sizeof(path))) {
                    usage->mounted = true; usage->system |= system_path((const char *)path);
                } else usage->complete = false;
            }
            CFRelease(description);
        }
        if (disk) CFRelease(disk);
    }
    CFRelease(session);
}
/** @brief Query cached mount metadata with MNT_NOWAIT and stat only reported block-node names. */
static int mount_table(const ds_inventory *inventory, const ds_usage_graph *graph, mac_usage *usage)
{
    int count = getfsstat(NULL, 0, MNT_NOWAIT);
    if (count <= 0 || count > (int)DS_USAGE_GRAPH_LIMIT - 16) { usage->complete = false; return 0; }
    size_t capacity = (size_t)count + 16U;
    if (capacity > (size_t)INT_MAX / sizeof(struct statfs)) return 1;
    struct statfs *mounts = calloc(capacity, sizeof(*mounts));
    if (!mounts) return 1;
    count = getfsstat(mounts, (int)(capacity * sizeof(*mounts)), MNT_NOWAIT);
    if (count < 0 || (size_t)count >= capacity) usage->complete = false;
    else for (int i = 0; i < count; ++i) {
        const struct statfs *m = &mounts[i];
        if (!memchr(m->f_mntfromname, '\0', sizeof(m->f_mntfromname)) ||
            !memchr(m->f_mntonname, '\0', sizeof(m->f_mntonname))) { usage->complete = false; continue; }
        if (strncmp(m->f_mntfromname, "/dev/", 5)) continue;
        struct stat st;
        if (stat(m->f_mntfromname, &st) != 0 || !S_ISBLK(st.st_mode)) { usage->complete = false; continue; }
        if (ds_usage_graph_devnum(inventory, graph, (unsigned int)major(st.st_rdev), (unsigned int)minor(st.st_rdev))) {
            usage->mounted = true; usage->system |= system_path(m->f_mntonname);
        }
    }
    free(mounts);
    return 0;
}
/** @brief Collect native usage within two registry snapshots while keeping APFS and boot scope unknown. */
int ds_collect_usage(const ds_device *device, ds_assessment *result, char *error, size_t size)
{
    ds_inventory before = {0}, after = {0}; ds_usage_graph first, last;
    int rc = ds_discover(&before, NULL, error, size);
    if (rc) goto done;
    rc = ds_usage_graph_build(&before, device, &first);
    if (rc) goto done;
    ds_target_status status;
    if (ds_target_compare(device, &before.items[first.selected], result, &status)) { rc = 1; goto done; }
    if (status == DS_TARGET_CHANGED) { rc = 3; goto done; }
    mac_usage usage = {false, false, true};
    arbitration_mounts(&before, &first, &usage);
    if (mount_table(&before, &first, &usage)) { rc = 1; goto done; }
/** @brief Preserve cleanup if bounded evidence cannot be retained. */
#define OBS(check, state, source, detail) do { if (ds_assessment_observe(result, check, state, source, detail)) { rc = 1; goto done; } } while (0)
    OBS(DS_CHECK_MOUNTED, usage.mounted ? DS_TRUE : DS_UNKNOWN, "macos:DiskArbitration+getfsstat", "Mounted volume paths and block identifiers are compared against observed registry relationships; absence is not idle media.");
    OBS(DS_CHECK_SYSTEM, usage.system ? DS_TRUE : DS_UNKNOWN, "macos:mount_paths", "Root or system-volume mount dependency observed when matched; boot, recovery and sealed/data-volume sharing remain incomplete.");
    OBS(DS_CHECK_SWAP, DS_UNKNOWN, "macos:swap_scope", "Active swap backing and shared APFS container dependencies are not exhaustively established.");
    OBS(DS_CHECK_TOPOLOGY, DS_UNKNOWN, "iokit:observed_graph", "Registry parent links do not fully resolve APFS sharing, snapshots, multi-device containers, Fusion or RAID backing.");
    OBS(DS_CHECK_USAGE, DS_UNKNOWN, "macos:usage_scope", "Open raw consumers and all APFS/storage-layer activity cannot be excluded by mount metadata.");
    OBS(DS_CHECK_HOST_SCOPE, DS_UNKNOWN, "macos:visibility_scope", "Accessible mount/registry observations are non-atomic and do not establish exclusive host-wide access.");
    OBS(DS_CHECK_PHYSICAL_SCOPE, DS_UNKNOWN, "macos:controller_scope", "IOMedia whole-device or namespace classification is not complete controller/physical operation scope.");
    OBS(DS_CHECK_COLLECTION, usage.complete && first.links_resolved ? DS_TRUE : DS_UNKNOWN, "macos:usage_collection", "Missing descriptions, unresolved links, bounded mount-table races or permissions remain incomplete observations.");
#undef OBS
    rc = ds_discover(&after, NULL, error, size);
    if (rc) goto done;
    rc = ds_usage_graph_build(&after, device, &last);
    if (!rc && !ds_usage_graph_same(&before, &first, &after, &last)) rc = 3;
done:
    ds_inventory_free(&before); ds_inventory_free(&after);
    if (rc) (void)snprintf(error, size, "%s", rc == 3 ? "Target usage graph changed or became ambiguous during inspection." : "Cannot collect native usage metadata.");
    return rc;
}
