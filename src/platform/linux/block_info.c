/**
 * @file
 * @brief Probe capacity and sector sizes through a closed read-only ioctl allowlist.
 */
#include "disksan/discovery.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

/** @brief Translate a saved ioctl errno into a warning without losing allocation failures. */
static int ioctl_warning(ds_device *d, int error)
{
    if (error == EACCES || error == EPERM) return ds_warn(d, "ioctl_permission_denied");
    if (error == ENODEV || error == ENXIO || error == ENOENT) return ds_warn(d, "device_disappeared");
    if (error == ENOTTY || error == EOPNOTSUPP || error == EINVAL) return ds_warn(d, "ioctl_unsupported");
    return ds_warn(d, "ioctl_failed");
}

/** @brief Open read-only, verify block identity, query three allowed ioctls, and always close the handle. */
int ds_linux_probe(ds_device *d)
{
    if (!d->path.value) return ds_warn(d, "devnode_unavailable");
    /* NONBLOCK prevents a replaced FIFO or unready removable device from hanging. */
    int fd = open(d->path.value, O_RDONLY | O_CLOEXEC | O_NONBLOCK | O_NOFOLLOW);
    if (fd < 0) {
        int saved = errno;
        if (saved == EACCES || saved == EPERM) return ds_warn(d, "devnode_permission_denied");
        if (saved == ENODEV || saved == ENXIO || saved == ENOENT) return ds_warn(d, "device_disappeared");
        return ds_warn(d, "devnode_open_failed");
    }
    struct stat st;
    int rc = 0;
    if (fstat(fd, &st) < 0 || !S_ISBLK(st.st_mode) ||
        major(st.st_rdev) != d->major_number || minor(st.st_rdev) != d->minor_number) {
        rc = ds_warn(d, "device_identity_changed");
        (void)close(fd);
        return rc;
    }
    /* Explicit, closed allowlist. No data-area reads or writes are performed. */
    uint64_t bytes = 0;
    if (ioctl(fd, BLKGETSIZE64, &bytes) == 0)
        rc = ds_number_offer(d, &d->size_bytes, bytes, "ioctl:BLKGETSIZE64");
    else { int saved = errno; rc = ioctl_warning(d, saved); }
    int logical = 0;
    if (rc == 0 && ioctl(fd, BLKSSZGET, &logical) == 0) {
        if (logical > 0) rc = ds_number_offer(d, &d->logical_sector_size, (uint64_t)logical, "ioctl:BLKSSZGET");
        else rc = ds_warn(d, "invalid_sector_size");
    } else if (rc == 0) { int saved = errno; rc = ioctl_warning(d, saved); }
    unsigned int physical = 0;
    if (rc == 0 && ioctl(fd, BLKPBSZGET, &physical) == 0) {
        if (physical > 0U) rc = ds_number_offer(d, &d->physical_sector_size, physical, "ioctl:BLKPBSZGET");
        else rc = ds_warn(d, "invalid_sector_size");
    } else if (rc == 0) { int saved = errno; rc = ioctl_warning(d, saved); }
    (void)close(fd);
    return rc;
}
