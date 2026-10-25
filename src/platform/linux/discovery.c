/** @file
 * @brief Enumerate Linux metadata records and distinguish partial scans from total failure.
 */
#include "disksan/discovery.h"
#include "native.h"
#include <errno.h>
#include <libudev.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <stdio.h>

/** @brief Write a bounded discovery error and return the caller-supplied exit code. */
static int fail(char *error, size_t size, int code, const char *message)
{
    (void)snprintf(error, size, "%s", message);
    return code;
}

/** @brief Enumerate Linux block devices or resolve one selector with identity revalidation. */
int ds_discover(ds_inventory *inventory, const char *selector, char *error, size_t size)
{
    struct stat selected;
    if (selector) {
        /* stat follows valid by-id links and rejects regular files before any open. */
        if (stat(selector, &selected) < 0) {
            int saved = errno;
            return fail(error, size, saved == EACCES || saved == EPERM ? 4 : 3,
                        "Cannot resolve target block device (missing, inaccessible, or broken symlink).");
        }
        if (!S_ISBLK(selected.st_mode)) return fail(error, size, 3, "Target is not a block device.");
    }
    struct udev *udev = udev_new();
    if (!udev) return fail(error, size, 1, "Cannot initialize libudev.");
    int rc = 0;
    if (selector) {
        errno = 0;
        struct udev_device *u = udev_device_new_from_devnum(udev, 'b', selected.st_rdev);
        int creation_error = errno;
        ds_device d;
        ds_device_init(&d);
        if (!u) rc = creation_error == ENOMEM ? fail(error, size, 1, "Memory allocation failed.") :
                    fail(error, size, 3, "Target has no current udev/sysfs block record.");
        else {
            int result = ds_linux_collect(u, &d);
            struct stat after;
            if (result < 0) rc = fail(error, size, 1, "Memory allocation failed.");
            else if (result > 0 || stat(selector, &after) < 0 || !S_ISBLK(after.st_mode) ||
                     after.st_rdev != selected.st_rdev ||
                     d.major_number != major(selected.st_rdev) || d.minor_number != minor(selected.st_rdev))
                rc = fail(error, size, 3, "Target disappeared or changed identity during discovery.");
            else if (ds_inventory_add(inventory, &d) < 0) rc = fail(error, size, 1, "Memory allocation failed.");
            udev_device_unref(u);
        }
        ds_device_free(&d);
    } else {
        struct udev_enumerate *enumeration = udev_enumerate_new(udev);
        if (!enumeration) rc = fail(error, size, 1, "Cannot allocate libudev enumeration.");
        else if (udev_enumerate_add_match_subsystem(enumeration, "block") < 0 ||
                 udev_enumerate_scan_devices(enumeration) < 0)
            rc = fail(error, size, 1, "Block enumeration failed; inventory is unavailable.");
        else {
            struct udev_list_entry *entry;
            udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(enumeration)) {
                errno = 0;
                struct udev_device *u = udev_device_new_from_syspath(udev, udev_list_entry_get_name(entry));
                int creation_error = errno;
                ds_device d;
                ds_device_init(&d);
                int result = u ? ds_linux_collect(u, &d) : creation_error == ENOMEM ? -1 : 1;
                if (u) udev_device_unref(u);
                if (result == 1) result = ds_strings_add(&inventory->warnings, "device_record_unavailable_or_changed");
                else if (result == 0) result = ds_inventory_add(inventory, &d);
                ds_device_free(&d);
                if (result < 0) { rc = fail(error, size, 1, result == -2 ? "Ambiguous device identity during discovery; retry." : "Memory allocation failed during discovery."); break; }
            }
            if (rc == 0 && inventory->count == 0U && inventory->warnings.count)
                rc = fail(error, size, 1, "All enumerated records became unavailable; inventory is incomplete.");
        }
        if (enumeration) udev_enumerate_unref(enumeration);
    }
    udev_unref(udev);
    return rc;
}
