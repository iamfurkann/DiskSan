/**
 * @file
 * @brief Observe namespace-local proc usage and revalidate the native storage graph read-only.
 */
#include "disksan/target.h"
#include "disksan/discovery.h"
#include "disksan/usage.h"
#include "usage_parse.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/magic.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/vfs.h>
#include <unistd.h>

/** @brief Bound the amount of proc metadata consumed during one observation. */
#define PROC_BYTES_LIMIT (16U * 1024U * 1024U)
/** @brief Accumulate only positive usage matches and completeness limitations. */
typedef struct {
    bool mounted;
    bool system;
    bool swap;
    bool mounts_complete;
    bool swaps_complete;
    bool inaccessible;
    bool backing_complete;
} usage_state;

/** @brief Validate regular procfs/sysfs metadata before reading; nonblocking open also rejects FIFO substitution promptly. */
static FILE *open_metadata(const char *path, long filesystem, bool *fatal)
{
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) { if (errno == ENOMEM) *fatal = true; return NULL; }
    struct stat st; struct statfs fs;
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || fstatfs(fd, &fs) != 0 || fs.f_type != filesystem) {
        (void)close(fd); return NULL;
    }
    FILE *stream = fdopen(fd, "r");
    if (!stream) { if (errno == ENOMEM) *fatal = true; (void)close(fd); }
    return stream;
}
/** @brief Read a bounded line including newline; distinguish clean EOF, malformed bytes and I/O failure. */
static int read_line(FILE *stream, char *line, size_t *length, size_t *total)
{
    *length = 0;
    for (;;) {
        int c = fgetc(stream);
        if (c == EOF) return ferror(stream) ? -1 : *length ? 1 : 0;
        if (++*total > PROC_BYTES_LIMIT || *length == DS_PROC_LINE_LIMIT || c == 0) return -1;
        line[(*length)++] = (char)c;
        if (c == '\n') return 1;
    }
}
/** @brief Recognize explicit root and common system mount locations without claiming boot coverage. */
static bool system_mount(const char *path)
{
    return !strcmp(path, "/") || !strcmp(path, "/boot") || !strcmp(path, "/boot/efi") ||
           !strcmp(path, "/usr") || !strcmp(path, "/var");
}
/** @brief Parse namespace-local mount identifiers and retain positive matches even on partial input. */
static int mounts(const ds_inventory *inventory, const ds_usage_graph *graph, usage_state *state)
{
    bool fatal = false;
    FILE *stream = open_metadata("/proc/self/mountinfo", PROC_SUPER_MAGIC, &fatal);
    if (!stream) { state->inaccessible = true; return fatal ? 1 : 0; }
    char line[DS_PROC_LINE_LIMIT]; size_t length, total = 0, records = 0;
    int rc;
    state->mounts_complete = true;
    while ((rc = read_line(stream, line, &length, &total)) == 1) {
        ds_mount_record record;
        if (ds_parse_mountinfo(line, length, &record)) { state->mounts_complete = false; continue; }
        ++records;
        if (ds_usage_graph_devnum(inventory, graph, record.major_number, record.minor_number)) {
            state->mounted = true;
            state->system |= system_mount(record.mount_point);
        }
    }
    if (rc < 0 || !records) state->mounts_complete = false;
    if (fclose(stream) != 0) state->mounts_complete = false;
    return 0;
}
/** @brief Resolve swap backing by stat metadata only, never reading a swap file or block data. */
static bool swap_device(const ds_swap_record *record, unsigned int *maj, unsigned int *min)
{
    struct stat before, after;
    if (stat(record->path, &before) != 0 || stat(record->path, &after) != 0 ||
        before.st_dev != after.st_dev || before.st_ino != after.st_ino || before.st_rdev != after.st_rdev ||
        (before.st_mode & S_IFMT) != (after.st_mode & S_IFMT)) return false;
    if (record->file ? !S_ISREG(before.st_mode) : !S_ISBLK(before.st_mode)) return false;
    dev_t number = record->file ? before.st_dev : before.st_rdev;
    *maj = major(number); *min = minor(number);
    return true;
}
/** @brief Parse active swap entries including escaped file paths and missing backing metadata. */
static int swaps(const ds_inventory *inventory, const ds_usage_graph *graph, usage_state *state)
{
    bool fatal = false;
    FILE *stream = open_metadata("/proc/swaps", PROC_SUPER_MAGIC, &fatal);
    if (!stream) { state->inaccessible = true; return fatal ? 1 : 0; }
    char line[DS_PROC_LINE_LIMIT]; size_t length, total = 0;
    int rc = read_line(stream, line, &length, &total);
    state->swaps_complete = rc == 1 && ds_swaps_header(line, length);
    if (state->swaps_complete) while ((rc = read_line(stream, line, &length, &total)) == 1) {
        ds_swap_record record; unsigned int maj, min;
        if (ds_parse_swap(line, length, &record) || !swap_device(&record, &maj, &min)) {
            state->swaps_complete = false; continue;
        }
        if (ds_usage_graph_devnum(inventory, graph, maj, min)) state->swap = true;
        /* A filesystem identifier without a visible block record has unresolved backing. */
        bool visible = false;
        for (size_t i = 0; i < inventory->count; ++i)
            if (inventory->items[i].has_devnum && inventory->items[i].major_number == maj && inventory->items[i].minor_number == min) visible = true;
        if (!visible) state->swaps_complete = false;
    }
    if (rc < 0) state->swaps_complete = false;
    if (fclose(stream) != 0) state->swaps_complete = false;
    return 0;
}
/** @brief Select only canonical loop sysfs attributes; the name is a probe hint, not capability evidence. */
static bool loop_attribute(const ds_device *device, char *path, size_t capacity)
{
    const char prefix[] = "/sys/devices/virtual/block/loop";
    const char *root = device->sysfs_path.value;
    uint64_t number;
    if (!root || strncmp(root, prefix, sizeof(prefix) - 1U) ||
        !ds_parse_u64(root + sizeof(prefix) - 1U, &number)) return false;
    int n = snprintf(path, capacity, "%s/loop/backing_file", root);
    return n > 0 && (size_t)n < capacity;
}
/** @brief Read only a verified sysfs attribute, preserving raw filename bytes and its final newline contract. */
static int backing_path(const char *attribute, char *path)
{
    bool fatal = false;
    FILE *stream = open_metadata(attribute, SYSFS_MAGIC, &fatal);
    if (!stream) return fatal ? -1 : 0;
    size_t length = 0; int c;
    while ((c = fgetc(stream)) != EOF && length < DS_TEXT_LIMIT + 1U) path[length++] = (char)c;
    bool valid = c == EOF && !ferror(stream) && length > 1U && path[0] == '/' &&
                 path[length - 1U] == '\n' && !memchr(path, '\0', length);
    if (fclose(stream) != 0) valid = false;
    if (!valid) { path[0] = '\0'; return 0; }
    path[length - 1U] = '\0';
    return 1;
}
/** @brief Match observed loop backing filenames by stat metadata, never opening the backing content. */
static int loop_backing(ds_inventory *inventory, usage_state *state)
{
    state->backing_complete = true;
    for (size_t i = 0; i < inventory->count; ++i) {
        ds_device *loop = &inventory->items[i];
        char attribute[DS_TEXT_LIMIT + 32U], path[DS_TEXT_LIMIT + 2U], again[DS_TEXT_LIMIT + 2U];
        if (!loop_attribute(loop, attribute, sizeof(attribute))) continue;
        int read = backing_path(attribute, path);
        if (read < 0) return 1;
        if (!read) { state->backing_complete = false; continue; }
        struct stat first, last;
        if (stat(path, &first) != 0 || stat(path, &last) != 0) { state->backing_complete = false; continue; }
        if (first.st_dev != last.st_dev || first.st_ino != last.st_ino || first.st_rdev != last.st_rdev ||
            (first.st_mode & S_IFMT) != (last.st_mode & S_IFMT)) return 3;
        read = backing_path(attribute, again);
        if (read < 0) return 1;
        if (!read || strcmp(path, again)) return 3;
        if (!S_ISREG(first.st_mode) && !S_ISBLK(first.st_mode)) { state->backing_complete = false; continue; }
        dev_t number = S_ISBLK(first.st_mode) ? first.st_rdev : first.st_dev;
        size_t match = SIZE_MAX;
        for (size_t j = 0; j < inventory->count; ++j) {
            const ds_device *candidate = &inventory->items[j];
            if (candidate->has_devnum && candidate->major_number == major(number) && candidate->minor_number == minor(number)) {
                if (match != SIZE_MAX) return 3;
                match = j;
            }
        }
        if (match == SIZE_MAX || match == i || !inventory->items[match].sysfs_path.value) {
            state->backing_complete = false; continue;
        }
        if (ds_strings_add(&loop->slaves, inventory->items[match].sysfs_path.value)) return 1;
    }
    return 0;
}
/** @brief Retain positive findings and explicit limits without interpreting absent rows as idle media. */
static int observations(ds_assessment *result, const ds_inventory *inventory, const ds_usage_graph *graph, const usage_state *state)
{
    bool holders = false, slaves = false, layered = false;
    for (size_t i = 0; i < graph->count; ++i) if (graph->related[i]) {
        const ds_device *d = &inventory->items[i];
        holders |= d->holders.count != 0U; slaves |= d->slaves.count != 0U;
        layered |= d->role && (!strcmp(d->role, "virtual") || !strcmp(d->role, "composite"));
    }
/** @brief Abort rather than silently dropping bounded evidence. */
#define OBS(check, state_value, source, detail) do { if (ds_assessment_observe(result, check, state_value, source, detail)) return 1; } while (0)
    OBS(DS_CHECK_MOUNTED, state->mounted ? DS_TRUE : DS_UNKNOWN, "proc:self/mountinfo", "Device-number matches include observed ancestors, descendants and shared-parent dependencies; absence is not host-wide inactivity.");
    OBS(DS_CHECK_SYSTEM, state->system ? DS_TRUE : DS_UNKNOWN, "proc:self/mountinfo", "Root, boot, usr or var mount dependency observed when matched; firmware boot and hidden namespaces remain unverified.");
    OBS(DS_CHECK_SWAP, state->swap ? DS_TRUE : DS_UNKNOWN, "proc:swaps+stat", "Active swap backing is matched using block st_rdev or regular-file st_dev; missing/layered filesystem backing remains unknown.");
    OBS(DS_CHECK_HOLDERS, holders ? DS_TRUE : DS_UNKNOWN, "sysfs:observed_graph", "Holder evidence is limited to visible native relationships.");
    OBS(DS_CHECK_SLAVES, slaves ? DS_TRUE : DS_UNKNOWN, "sysfs:observed_graph+loop_backing_file+stat", "Backing links include visible slaves and loop filenames mapped by stat; a pathname is not proof of the kernel's held backing inode.");
    OBS(DS_CHECK_TOPOLOGY, DS_UNKNOWN, "sysfs:observed_graph", !graph->links_resolved ? "One or more explicit relationships could not be resolved." :
        layered ? "Layered/virtual storage is present; loop files, RAID, DM and multipath may hide physical backing." : "Explicit observed links resolve, but hidden consumers and controller scope are not completely established.");
    OBS(DS_CHECK_USAGE, DS_UNKNOWN, "linux:usage_scope", "Open raw-device users, hidden mounts and storage-layer consumers are not exhaustively observable.");
    OBS(DS_CHECK_HOST_SCOPE, DS_UNKNOWN, "proc:self/mountinfo", "Only the current process mount namespace is visible; no host-wide exclusivity is established.");
    OBS(DS_CHECK_PHYSICAL_SCOPE, DS_UNKNOWN, "linux:controller_scope", "A block device or namespace does not establish the complete physical/controller operation scope.");
    OBS(DS_CHECK_COLLECTION, state->mounts_complete && state->swaps_complete && state->backing_complete && graph->links_resolved ? DS_TRUE : DS_UNKNOWN,
        "linux:usage_collection", state->inaccessible ? "A proc metadata source was inaccessible or failed validation; no elevation was attempted." :
        "Parsing completeness applies only to the bounded visible metadata, not all possible device uses.");
#undef OBS
    return 0;
}
/** @brief Collect usage between native graph snapshots and reject observable graph changes. */
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
    usage_state state = {0};
    rc = loop_backing(&before, &state);
    if (rc) goto done;
    rc = ds_usage_graph_build(&before, device, &first);
    if (rc) goto done;
    if (mounts(&before, &first, &state) || swaps(&before, &first, &state) || observations(result, &before, &first, &state)) { rc = 1; goto done; }
    rc = ds_discover(&after, NULL, error, size);
    if (rc) goto done;
    if (after.count > DS_USAGE_GRAPH_LIMIT) { rc = 1; goto done; }
    usage_state rechecked = {0};
    rc = loop_backing(&after, &rechecked);
    if (rc) goto done;
    rc = ds_usage_graph_build(&after, device, &last);
    if (!rc && !ds_usage_graph_same(&before, &first, &after, &last)) rc = 3;
done:
    ds_inventory_free(&before); ds_inventory_free(&after);
    if (rc) (void)snprintf(error, size, "%s", rc == 3 ? "Target usage graph changed or became ambiguous during inspection." : "Cannot collect the bounded native usage graph.");
    return rc;
}
