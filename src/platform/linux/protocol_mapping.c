/** @file
 * @brief Verify a direct NVMe namespace/controller association using fresh bounded udev metadata and node identity.
 */
#include "protocol_mapping.h"
#include <errno.h>
#include <inttypes.h>
#include <libudev.h>
#include <string.h>
#include <sys/sysmacros.h>

/** @brief Compare optional strings without accepting missing identity. */
static bool equal(const char *left, const char *right)
{
    return left && right && strcmp(left, right) == 0;
}
/** @brief Reject missing, empty and oversized path metadata before copying or native node inspection. */
static bool bounded_path(const char *path, const char *prefix)
{
    return path && strnlen(path, DS_TEXT_LIMIT + 1U) <= DS_TEXT_LIMIT &&
           strncmp(path, prefix, strlen(prefix)) == 0 && path[strlen(prefix)] != '\0';
}
/** @brief Parse only a fixed small numeric sysfs attribute; no caller-selected attribute is exposed. */
static bool number(struct udev_device *device, const char *name, uint64_t maximum, uint64_t *value)
{
    const char *text = udev_device_get_sysattr_value(device, name);
    return text && strnlen(text, 32U) < 32U && ds_parse_u64(text, value) && *value <= maximum;
}
/** @brief Compare node kind and filesystem identity as well as the kernel device number. */
static bool same_node(const struct stat *left, const struct stat *right)
{
    return (left->st_mode & S_IFMT) == (right->st_mode & S_IFMT) &&
           left->st_rdev == right->st_rdev && left->st_dev == right->st_dev && left->st_ino == right->st_ino;
}
/** @brief Recreate native objects for each observation so libudev attribute caching cannot reuse prior evidence. */
ds_linux_map_status ds_linux_protocol_map(const ds_device *device, ds_linux_protocol_mapping *mapping)
{
    memset(mapping, 0, sizeof(*mapping));
    if (!device->has_devnum || !bounded_path(device->path.value, "/dev/") ||
        !bounded_path(device->sysfs_path.value, "/sys/devices/") ||
        !bounded_path(device->controller.value, "/sys/devices/") ||
        !device->device_id.value || !equal(device->device_id.source, "sysfs:syspath+diskseq"))
        return DS_LINUX_MAP_UNKNOWN;
    dev_t selected = makedev(device->major_number, device->minor_number);
    if (major(selected) != device->major_number || minor(selected) != device->minor_number)
        return DS_LINUX_MAP_CHANGED;
    struct udev *context = udev_new();
    if (!context) return DS_LINUX_MAP_ERROR;
    errno = 0;
    struct udev_device *ns = udev_device_new_from_devnum(context, 'b', selected);
    ds_linux_map_status rc = errno == ENOMEM ? DS_LINUX_MAP_ERROR : DS_LINUX_MAP_CHANGED;
    if (!ns) goto done;
    if (!equal(udev_device_get_subsystem(ns), "block") || !equal(udev_device_get_devtype(ns), "disk") ||
        udev_device_get_devnum(ns) != selected || !equal(udev_device_get_syspath(ns), device->sysfs_path.value) ||
        !equal(udev_device_get_devnode(ns), device->path.value)) { rc = DS_LINUX_MAP_CHANGED; goto done; }
    /* A namespace-head/aggregate or translated path cannot substitute for the direct controller parent. */
    struct udev_device *ctrl = udev_device_get_parent(ns);
    struct udev_device *pci = ctrl ? udev_device_get_parent(ctrl) : NULL;
    rc = DS_LINUX_MAP_UNKNOWN;
    if (!ctrl || !pci || !equal(udev_device_get_subsystem(ctrl), "nvme") ||
        !equal(udev_device_get_subsystem(pci), "pci") || !equal(udev_device_get_driver(pci), "nvme") ||
        !equal(udev_device_get_sysattr_value(ctrl, "transport"), "pcie") ||
        !equal(udev_device_get_sysattr_value(ctrl, "state"), "live")) goto done;
    if (!equal(udev_device_get_syspath(ctrl), device->controller.value)) { rc = DS_LINUX_MAP_CHANGED; goto done; }
    uint64_t sequence = 0, nsid = 0, cntlid = 0;
    if (!number(ns, "diskseq", UINT64_MAX, &sequence) || sequence == 0U ||
        !number(ns, "nsid", UINT32_MAX - 1U, &nsid) || nsid == 0U ||
        !number(ctrl, "cntlid", UINT16_MAX - 1U, &cntlid)) goto done;
    char identity[DS_TEXT_LIMIT + 1U];
    int length = snprintf(identity, sizeof(identity), "%s#diskseq=%" PRIu64, device->sysfs_path.value, sequence);
    if (length < 0 || (size_t)length >= sizeof(identity)) goto done;
    if (!equal(identity, device->device_id.value)) { rc = DS_LINUX_MAP_CHANGED; goto done; }
    const char *node = udev_device_get_devnode(ctrl);
    if (!bounded_path(node, "/dev/")) goto done;
    memcpy(mapping->controller_node, node, strlen(node) + 1U);
    dev_t control = udev_device_get_devnum(ctrl);
    if (major(control) == 0U) goto done;
    /* No symlink traversal: the discovery selector was already normalized to its udev block node. */
    rc = DS_LINUX_MAP_CHANGED;
    if (lstat(device->path.value, &mapping->namespace_stat) ||
        !S_ISBLK(mapping->namespace_stat.st_mode) || mapping->namespace_stat.st_rdev != selected ||
        lstat(node, &mapping->controller_stat) || !S_ISCHR(mapping->controller_stat.st_mode) ||
        mapping->controller_stat.st_rdev != control) goto done;
    mapping->diskseq = sequence; mapping->nsid = (uint32_t)nsid; mapping->cntlid = (uint16_t)cntlid;
    rc = DS_LINUX_MAP_OK;
done:
    if (ns) udev_device_unref(ns);
    udev_unref(context);
    if (rc != DS_LINUX_MAP_OK) memset(mapping, 0, sizeof(*mapping));
    return rc;
}
/** @brief Match both independently mapped nodes and numeric controller/namespace facts. */
bool ds_linux_protocol_mapping_equal(const ds_linux_protocol_mapping *before, const ds_linux_protocol_mapping *after)
{
    return equal(before->controller_node, after->controller_node) &&
           same_node(&before->controller_stat, &after->controller_stat) &&
           same_node(&before->namespace_stat, &after->namespace_stat) &&
           before->diskseq == after->diskseq && before->nsid == after->nsid && before->cntlid == after->cntlid;
}
/** @brief Require the pinned descriptor to refer to the exact observed character node. */
bool ds_linux_protocol_descriptor_matches(const ds_linux_protocol_mapping *mapping, const struct stat *descriptor_stat)
{
    return S_ISCHR(descriptor_stat->st_mode) && same_node(&mapping->controller_stat, descriptor_stat);
}
