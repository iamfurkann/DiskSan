/**
 * @file
 * @brief Build Linux snapshots from libudev, bounded sysfs reads, and read-only probes.
 */
#include "disksan/discovery.h"
#include <dirent.h>
#include <errno.h>
#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

/** @brief Compare an optional metadata string with a required reference string. */
static bool equals(const char *a, const char *b) { return a && strcmp(a, b) == 0; }

/* Read only bounded sysfs attributes. Missing optional attributes are normal. */
/** @brief Read a bounded sysfs attribute; return 1 for data, 0 for absence, or -1 for allocation failure. */
static int attribute(ds_device *d, const char *root, const char *name, char *value)
{
    if (!root) return 0;
    char path[DS_TEXT_LIMIT * 2U + 2U];
    int n = snprintf(path, sizeof(path), "%s/%s", root, name);
    if (n < 0 || (size_t)n >= sizeof(path)) return ds_warn(d, "metadata_path_too_long") < 0 ? -1 : 0;
    FILE *file = fopen(path, "r");
    if (!file) {
        int saved = errno;
        if (saved != ENOENT && saved != ENOTDIR && ds_warn(d, "sysfs_read_failed") < 0) return -1;
        return 0;
    }
    size_t len = fread(value, 1, DS_TEXT_LIMIT, file);
    bool failed = ferror(file) != 0;
    int extra = !failed && len == DS_TEXT_LIMIT ? fgetc(file) : EOF;
    failed = failed || ferror(file) != 0;
    (void)fclose(file);
    if (failed || extra != EOF || memchr(value, '\0', len)) {
        return ds_warn(d, "invalid_sysfs_metadata") < 0 ? -1 : 0;
    }
    while (len && (value[len - 1U] == '\n' || value[len - 1U] == '\r' || value[len - 1U] == ' ')) --len;
    value[len] = '\0';
    return len ? 1 : 0;
}

/** @brief Offer an available sysfs string to the record using the supplied evidence source. */
static int text_attribute(ds_device *d, ds_text *field, const char *root, const char *name, const char *source)
{
    char value[DS_TEXT_LIMIT + 1U];
    int rc = attribute(d, root, name, value);
    if (rc <= 0) return rc;
    return ds_text_offer(d, field, value, source);
}

/** @brief Validate a sysfs number as an integer, boolean, sector count, or positive sector size. */
static int number_attribute(ds_device *d, ds_number *field, const char *root,
                            const char *name, const char *source, int kind)
{
    char value[DS_TEXT_LIMIT + 1U];
    int rc = attribute(d, root, name, value);
    if (rc <= 0) return rc;
    uint64_t number;
    bool valid = kind == 2 ? ds_sectors_to_bytes(value, &number) : ds_parse_u64(value, &number);
    if (!valid || (kind == 1 && number > 1U) || (kind == 3 && (number == 0U || number > UINT32_MAX)))
        return ds_warn(d, "invalid_numeric_metadata");
    return ds_number_offer(d, field, number, source);
}

/** @brief Check a bounded sysfs child path for a directory used as classification evidence. */
static bool has_directory(const char *root, const char *name)
{
    char path[DS_TEXT_LIMIT * 2U + 2U];
    int n = snprintf(path, sizeof(path), "%s/%s", root, name);
    struct stat st;
    return n > 0 && (size_t)n < sizeof(path) && stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

/** @brief Resolve holder or slave links while reporting missing or changing topology. */
static int relations(ds_device *d, const char *root, const char *name, ds_strings *list)
{
    char path[DS_TEXT_LIMIT * 2U + 2U];
    int n = snprintf(path, sizeof(path), "%s/%s", root, name);
    if (n < 0 || (size_t)n >= sizeof(path)) return ds_warn(d, "topology_unavailable");
    DIR *dir = opendir(path);
    if (!dir) {
        int saved = errno;
        return saved == ENOENT ? 0 : ds_warn(d, "topology_unavailable");
    }
    int rc = 0;
    for (;;) {
        errno = 0;
        struct dirent *entry = readdir(dir);
        if (!entry) {
            if (errno) rc = ds_warn(d, "topology_unavailable");
            break;
        }
        if (entry->d_name[0] == '.') continue;
        char link[DS_TEXT_LIMIT * 2U + 2U];
        n = snprintf(link, sizeof(link), "%s/%s", path, entry->d_name);
        if (n < 0 || (size_t)n >= sizeof(link)) { rc = ds_warn(d, "topology_unavailable"); }
        else {
            char *resolved = realpath(link, NULL);
            if (!resolved) rc = ds_warn(d, "topology_changed");
            else { rc = ds_strings_add(list, resolved); free(resolved); }
        }
        if (rc < 0) break;
    }
    (void)closedir(dir);
    return rc;
}

/** @brief Retain only by-id links whose resolved block number matches the current record. */
static int stable_links(ds_device *d, struct udev_device *u)
{
    struct udev_list_entry *entry;
    udev_list_entry_foreach(entry, udev_device_get_devlinks_list_entry(u)) {
        const char *path = udev_list_entry_get_name(entry);
        if (!path || strncmp(path, "/dev/disk/by-id/", 16) != 0) continue;
        struct stat st;
        if (stat(path, &st) == 0 && S_ISBLK(st.st_mode) &&
            major(st.st_rdev) == d->major_number && minor(st.st_rdev) == d->minor_number) {
            if (ds_strings_add(&d->stable_paths, path) < 0) return -1;
        } else if (ds_warn(d, "stable_path_stale_or_mismatched") < 0) return -1;
    }
    return 0;
}

/** @brief Compare sysfs dev metadata with the record; return -1 if warning allocation fails. */
static int devnum_matches(ds_device *d, const char *root)
{
    char value[DS_TEXT_LIMIT + 1U], expected[64];
    int rc = attribute(d, root, "dev", value);
    if (rc != 1) return rc;
    (void)snprintf(expected, sizeof(expected), "%u:%u", d->major_number, d->minor_number);
    return strcmp(value, expected) == 0;
}

/* 0 collected; 1 disappeared/inconsistent identity; -1 allocation failure. */
/** @brief Collect one block record; return 0 on success, 1 for changed identity, or -1 for allocation failure. */
static int collect(struct udev_device *u, ds_device *d)
{
    const char *root = udev_device_get_syspath(u);
    if (!root || !equals(udev_device_get_subsystem(u), "block")) return 1;
    dev_t number = udev_device_get_devnum(u);
    d->platform = "linux";
    d->has_devnum = true;
    d->major_number = major(number);
    d->minor_number = minor(number);
    int matched = devnum_matches(d, root);
    if (matched < 0) return -1;
    if (!matched) return 1;
    char diskseq[DS_TEXT_LIMIT + 1U], diskseq_after[DS_TEXT_LIMIT + 1U];
    int seq_present = attribute(d, root, "diskseq", diskseq);
    if (seq_present < 0) return -1;
/** @brief Propagate metadata allocation failure through the local cleanup policy. */
#define TRY(expr) do { if ((expr) < 0) return -1; } while (0)
    TRY(ds_text_offer(d, &d->sysfs_path, root, "udev:syspath"));
    TRY(ds_text_offer(d, &d->device_id, root, "udev:syspath"));
    TRY(ds_text_offer(d, &d->path, udev_device_get_devnode(u), "udev:devnode"));
    ds_facts facts = {0};
    facts.partition = equals(udev_device_get_devtype(u), "partition");
    facts.disk = equals(udev_device_get_devtype(u), "disk");
    facts.composite = has_directory(root, "dm") || has_directory(root, "md");
    facts.virtual_device = strstr(root, "/virtual/block/") != NULL;
    facts.ata = equals(udev_device_get_property_value(u, "ID_ATA"), "1");
    facts.optical = equals(udev_device_get_property_value(u, "ID_CDROM"), "1");
    const char *bus = udev_device_get_property_value(u, "ID_BUS");
    facts.usb = equals(bus, "usb");
    facts.ata = facts.ata || equals(bus, "ata");
    facts.scsi = equals(bus, "scsi");
    struct udev_device *whole = u;
    if (facts.partition) {
        whole = udev_device_get_parent_with_subsystem_devtype(u, "block", "disk");
        if (!whole) whole = u;
        else TRY(ds_text_offer(d, &d->parent, udev_device_get_syspath(whole), "udev:block_parent"));
    }
    const char *whole_root = udev_device_get_syspath(whole);
    char scsi_type[DS_TEXT_LIMIT + 1U];
    int type_present = attribute(d, whole_root, "device/type", scsi_type);
    if (type_present < 0) return -1;
    if (type_present == 1 && strcmp(scsi_type, "5") == 0) facts.optical = true;
    const char *controller_root = NULL;
    for (struct udev_device *p = u; p; p = udev_device_get_parent(p)) {
        const char *subsystem = udev_device_get_subsystem(p);
        facts.usb = facts.usb || equals(subsystem, "usb");
        facts.virtio = facts.virtio || equals(subsystem, "virtio");
        facts.scsi = facts.scsi || equals(subsystem, "scsi");
        facts.mmc = facts.mmc || equals(subsystem, "mmc");
        if (equals(subsystem, "nvme-subsystem")) facts.composite = true;
        if (equals(subsystem, "xen")) facts.virtual_device = true;
        if (equals(subsystem, "nvme")) {
            facts.nvme = true;
            controller_root = udev_device_get_syspath(p);
            TRY(ds_text_offer(d, &d->controller, controller_root, "udev:nvme_parent"));
        }
    }
    if (!facts.ata) facts.ata = equals(udev_device_get_property_value(whole, "ID_ATA"), "1");
    /* Kernel identity attributes precede udev. Never use ID_SERIAL's composite string as a serial. */
    TRY(text_attribute(d, &d->model, whole_root, "device/model", "sysfs:device/model"));
    TRY(text_attribute(d, &d->vendor, whole_root, "device/vendor", "sysfs:device/vendor"));
    TRY(text_attribute(d, &d->serial, whole_root, "device/serial", "sysfs:device/serial"));
    TRY(text_attribute(d, &d->model, controller_root, "model", "sysfs:nvme/model"));
    TRY(text_attribute(d, &d->serial, controller_root, "serial", "sysfs:nvme/serial"));
    /* ID_MODEL uses underscores; take it only when kernel display text is absent. */
    if (!d->model.value) TRY(ds_text_offer(d, &d->model, udev_device_get_property_value(whole, "ID_MODEL"), "udev:ID_MODEL"));
    if (!d->vendor.value) TRY(ds_text_offer(d, &d->vendor, udev_device_get_property_value(whole, "ID_VENDOR"), "udev:ID_VENDOR"));
    TRY(ds_text_offer(d, &d->serial, udev_device_get_property_value(whole, "ID_SERIAL_SHORT"), "udev:ID_SERIAL_SHORT"));
    TRY(ds_text_offer(d, &d->wwn, udev_device_get_property_value(whole, "ID_WWN_WITH_EXTENSION"), "udev:ID_WWN_WITH_EXTENSION"));
    if (!d->wwn.value) TRY(ds_text_offer(d, &d->wwn, udev_device_get_property_value(whole, "ID_WWN"), "udev:ID_WWN"));
    TRY(text_attribute(d, &d->eui, whole_root, "eui", "sysfs:eui"));
    TRY(text_attribute(d, &d->nguid, whole_root, "nguid", "sysfs:nguid"));
    TRY(relations(d, root, "holders", &d->holders));
    TRY(relations(d, root, "slaves", &d->slaves));
    if (d->slaves.count) facts.composite = true;
    TRY(stable_links(d, u));
    /* Read-only ioctl values precede sysfs fallbacks. Conflicts remain visible. */
    TRY(ds_linux_probe(d));
    TRY(number_attribute(d, &d->size_bytes, root, "size", "sysfs:size*512", 2));
    TRY(number_attribute(d, &d->logical_sector_size, whole_root, "queue/logical_block_size", "sysfs:queue/logical_block_size", 3));
    TRY(number_attribute(d, &d->physical_sector_size, whole_root, "queue/physical_block_size", "sysfs:queue/physical_block_size", 3));
    TRY(number_attribute(d, &d->rotational, whole_root, "queue/rotational", "sysfs:queue/rotational", 1));
    TRY(number_attribute(d, &d->removable, whole_root, "removable", "sysfs:removable", 1));
    TRY(number_attribute(d, &d->read_only, root, "ro", "sysfs:ro", 1));
    TRY(ds_classify(d, &facts));
    d->classification_source = "udev_ancestry_and_sysfs";
    if (!d->serial.value && !d->wwn.value && !d->eui.value && !d->nguid.value) TRY(ds_warn(d, "persistent_identity_unavailable"));
    else TRY(ds_warn(d, "identity_uniqueness_unverified"));
    if (!d->size_bytes.known) TRY(ds_warn(d, "capacity_unavailable"));
    if (!d->logical_sector_size.known || !d->physical_sector_size.known) TRY(ds_warn(d, "sector_size_unavailable"));
    if (!d->rotational.known || !d->removable.known || !d->read_only.known) TRY(ds_warn(d, "flags_unavailable"));
    matched = devnum_matches(d, root);
    if (matched < 0) return -1;
    if (!matched) return 1;
    if (seq_present == 1) {
        int after = attribute(d, root, "diskseq", diskseq_after);
        if (after < 0) return -1;
        if (after != 1 || strcmp(diskseq, diskseq_after) != 0) return 1;
    }
#undef TRY
    return 0;
}

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
            int result = collect(u, &d);
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
                int result = u ? collect(u, &d) : creation_error == ENOMEM ? -1 : 1;
                if (u) udev_device_unref(u);
                if (result == 1) result = ds_strings_add(&inventory->warnings, "device_record_unavailable_or_changed");
                else if (result == 0) result = ds_inventory_add(inventory, &d);
                ds_device_free(&d);
                if (result < 0) { rc = fail(error, size, 1, "Memory allocation failed during discovery."); break; }
            }
            if (rc == 0 && inventory->count == 0U && inventory->warnings.count)
                rc = fail(error, size, 1, "All enumerated records became unavailable; inventory is incomplete.");
        }
        if (enumeration) udev_enumerate_unref(enumeration);
    }
    udev_unref(udev);
    return rc;
}
