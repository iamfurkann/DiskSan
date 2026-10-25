/**
 * @file
 * @brief Discover IOMedia metadata through IOKit and Disk Arbitration without opening disks.
 */
#include "disksan/discovery.h"
#include <CoreFoundation/CoreFoundation.h>
#include <DiskArbitration/DiskArbitration.h>
#include <IOKit/IOBSD.h>
#include <IOKit/IOKitLib.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/** @brief Borrow a dictionary value, returning null when the optional dictionary is absent. */
static CFTypeRef value(CFDictionaryRef dict, CFStringRef key)
{
    return dict ? CFDictionaryGetValue(dict, key) : NULL;
}

/** @brief Compare a Core Foundation object only when its runtime type is a string. */
static bool text_is(CFTypeRef object, CFStringRef expected)
{
    return object && CFGetTypeID(object) == CFStringGetTypeID() && CFEqual(object, expected);
}

/** @brief Read a nonnegative signed Core Foundation number into an unsigned output value. */
static bool number(CFTypeRef object, uint64_t *result)
{
    int64_t n;
    if (!object || CFGetTypeID(object) != CFNumberGetTypeID() ||
        !CFNumberGetValue(object, kCFNumberSInt64Type, &n) || n < 0) return false;
    *result = (uint64_t)n;
    return true;
}

/** @brief Convert a bounded native string to UTF-8 and preserve its evidence source. */
static int offer_text(ds_device *d, ds_text *field, CFTypeRef object, const char *source)
{
    if (!object) return 0;
    if (CFGetTypeID(object) != CFStringGetTypeID()) return ds_warn(d, "invalid_native_metadata");
    char text[DS_TEXT_LIMIT + 1U];
    if (!CFStringGetCString(object, text, sizeof(text), kCFStringEncodingUTF8))
        return ds_warn(d, "metadata_too_long");
    return ds_text_offer(d, field, text, source);
}

/** @brief Offer a nonnegative native number or record an invalid-metadata warning. */
static int offer_number(ds_device *d, ds_number *field, CFTypeRef object, const char *source)
{
    uint64_t n;
    if (!object) return 0;
    return number(object, &n) ? ds_number_offer(d, field, n, source) : ds_warn(d, "invalid_native_metadata");
}

/** @brief Offer a typed native boolean, optionally inverting a writable flag into read-only. */
static int offer_bool(ds_device *d, ds_number *field, CFTypeRef object, const char *source, bool invert)
{
    if (!object) return 0;
    if (CFGetTypeID(object) != CFBooleanGetTypeID()) return ds_warn(d, "invalid_native_metadata");
    bool b = CFBooleanGetValue(object) != 0;
    return ds_number_offer(d, field, (uint64_t)(invert ? !b : b), source);
}

/** @brief Copy the nearest matching registry dictionary; the caller must CFRelease it. */
static CFDictionaryRef parent_dictionary(io_service_t media, CFStringRef key)
{
    CFTypeRef object = IORegistryEntrySearchCFProperty(media, kIOServicePlane, key,
        kCFAllocatorDefault, kIORegistryIterateRecursively | kIORegistryIterateParents);
    if (object && CFGetTypeID(object) != CFDictionaryGetTypeID()) { CFRelease(object); return NULL; }
    return object;
}

/* Nearest observed IOMedia parent, not an assumed complete physical topology. */
/** @brief Record the nearest observed IOMedia parent without claiming complete physical topology. */
static int parent_path(ds_device *d, io_service_t media)
{
    io_registry_entry_t current = IO_OBJECT_NULL;
    if (IORegistryEntryGetParentEntry(media, kIOServicePlane, &current) != KERN_SUCCESS) return 0;
    int result = 0;
    for (unsigned int depth = 0; current && depth < 128U; ++depth) {
        if (IOObjectConformsTo(current, "IOMedia")) {
            CFTypeRef name = IORegistryEntryCreateCFProperty(current, CFSTR(kIOBSDNameKey), kCFAllocatorDefault, 0);
            char bsd[256], path[sizeof(bsd) + 6U];
            if (name && CFGetTypeID(name) == CFStringGetTypeID() &&
                CFStringGetCString(name, bsd, sizeof(bsd), kCFStringEncodingUTF8)) {
                (void)snprintf(path, sizeof(path), "/dev/%s", bsd);
                result = ds_text_offer(d, &d->parent, path, "iokit:nearest_media_parent");
                CFRelease(name);
                break;
            }
            if (name) CFRelease(name);
        }
        io_registry_entry_t next = IO_OBJECT_NULL;
        (void)IORegistryEntryGetParentEntry(current, kIOServicePlane, &next);
        IOObjectRelease(current);
        current = next;
    }
    if (current) IOObjectRelease(current);
    return result;
}

/* All access is via read-only registry / description APIs: no device open. */
/** @brief Collect one IOMedia record and recheck its registry identity before releasing native objects. */
static int collect(io_service_t media, DASessionRef session, ds_device *d)
{
    CFMutableDictionaryRef props = NULL;
    if (IORegistryEntryCreateCFProperties(media, &props, kCFAllocatorDefault, 0) != KERN_SUCCESS) return 1;
    CFDictionaryRef characteristics = parent_dictionary(media, CFSTR("Device Characteristics"));
    CFDictionaryRef protocol = parent_dictionary(media, CFSTR("Protocol Characteristics"));
    DADiskRef disk = DADiskCreateFromIOMedia(kCFAllocatorDefault, session, media);
    CFDictionaryRef description = disk ? DADiskCopyDescription(disk) : NULL;
    int rc = 0;
/** @brief Propagate metadata allocation failure through the local cleanup policy. */
#define TRY(expr) do { if ((expr) < 0) { rc = -1; goto done; } } while (0)
    char bsd[256], path[sizeof(bsd) + 6U], native_id[80];
    CFTypeRef name = value(props, CFSTR(kIOBSDNameKey));
    if (!name || CFGetTypeID(name) != CFStringGetTypeID() ||
        !CFStringGetCString(name, bsd, sizeof(bsd), kCFStringEncodingUTF8)) { rc = 1; goto done; }
    (void)snprintf(path, sizeof(path), "/dev/%s", bsd);
    uint64_t registry_id, maj, min;
    if (IORegistryEntryGetRegistryEntryID(media, &registry_id) != KERN_SUCCESS ||
        !number(value(props, CFSTR(kIOBSDMajorKey)), &maj) || maj > UINT_MAX ||
        !number(value(props, CFSTR(kIOBSDMinorKey)), &min) || min > UINT_MAX) { rc = 1; goto done; }
    d->platform = "macos";
    d->has_devnum = true;
    d->major_number = (unsigned int)maj;
    d->minor_number = (unsigned int)min;
    (void)snprintf(native_id, sizeof(native_id), "iokit:%" PRIu64, registry_id);
    TRY(ds_text_offer(d, &d->device_id, native_id, "iokit:registry_entry_id"));
    TRY(ds_text_offer(d, &d->path, path, "iokit:bsd_name"));
    TRY(offer_number(d, &d->size_bytes, value(props, CFSTR("Size")), "iokit:Size"));
    TRY(offer_number(d, &d->logical_sector_size, value(props, CFSTR("Preferred Block Size")), "iokit:Preferred Block Size"));
    TRY(offer_number(d, &d->physical_sector_size, value(props, CFSTR("Physical Block Size")), "iokit:Physical Block Size"));
    TRY(offer_bool(d, &d->removable, value(props, CFSTR("Removable")), "iokit:Removable", false));
    TRY(offer_bool(d, &d->read_only, value(props, CFSTR("Writable")), "iokit:Writable(inverted)", true));
    TRY(offer_text(d, &d->model, value(characteristics, CFSTR("Product Name")), "iokit:Product Name"));
    TRY(offer_text(d, &d->vendor, value(characteristics, CFSTR("Vendor Name")), "iokit:Vendor Name"));
    TRY(offer_text(d, &d->serial, value(characteristics, CFSTR("Serial Number")), "iokit:Serial Number"));
    if (!d->model.value) TRY(offer_text(d, &d->model, value(description, kDADiskDescriptionDeviceModelKey), "diskarbitration:DeviceModel"));
    if (!d->vendor.value) TRY(offer_text(d, &d->vendor, value(description, kDADiskDescriptionDeviceVendorKey), "diskarbitration:DeviceVendor"));
    if (!description) TRY(ds_warn(d, "diskarbitration_description_unavailable"));
    ds_facts facts = {0};
    CFTypeRef whole = value(props, CFSTR("Whole"));
    facts.disk = whole && CFGetTypeID(whole) == CFBooleanGetTypeID() && CFBooleanGetValue(whole);
    facts.partition = whole && CFGetTypeID(whole) == CFBooleanGetTypeID() && !CFBooleanGetValue(whole);
    facts.snapshot = IOObjectConformsTo(media, "AppleAPFSSnapshot");
    facts.volume = IOObjectConformsTo(media, "AppleAPFSVolume");
    facts.composite = !facts.volume && !facts.snapshot && IOObjectConformsTo(media, "AppleAPFSMedia");
    CFTypeRef transport = value(protocol, CFSTR("Physical Interconnect"));
    if (!transport) transport = value(description, kDADiskDescriptionDeviceProtocolKey);
    facts.usb = text_is(transport, CFSTR("USB"));
    facts.nvme = text_is(transport, CFSTR("NVMe")) || text_is(transport, CFSTR("NVM Express"));
    facts.ata = text_is(transport, CFSTR("ATA")) || text_is(transport, CFSTR("SATA")) || text_is(transport, CFSTR("Serial ATA"));
    facts.scsi = text_is(transport, CFSTR("SCSI")) || text_is(transport, CFSTR("SAS"));
    facts.virtual_device = text_is(transport, CFSTR("Virtual Interface"));
    CFTypeRef medium = value(characteristics, CFSTR("Medium Type"));
    if (text_is(medium, CFSTR("Solid State")) || text_is(medium, CFSTR("Rotational"))) {
        facts.media_reported = true;
        TRY(ds_number_offer(d, &d->rotational, text_is(medium, CFSTR("Rotational")) ? 1U : 0U, "iokit:Medium Type"));
    }
    TRY(ds_classify(d, &facts));
    d->classification_source = "iokit_and_diskarbitration";
    TRY(parent_path(d, media));
    if (facts.composite || facts.volume || facts.snapshot) TRY(ds_warn(d, "topology_is_registry_parent_only"));
    if (!d->serial.value) TRY(ds_warn(d, "persistent_identity_unavailable"));
    else TRY(ds_warn(d, "identity_uniqueness_unverified"));
    if (!d->physical_sector_size.known) TRY(ds_warn(d, "physical_sector_size_unavailable"));
    if (!d->size_bytes.known) TRY(ds_warn(d, "capacity_unavailable"));
    /* Re-resolve the BSD node against the current registry to detect replacement. */
    io_service_t now = IOServiceGetMatchingService(kIOMainPortDefault, IOBSDNameMatching(kIOMainPortDefault, 0, bsd));
    uint64_t current_id = 0;
    bool same = now && IORegistryEntryGetRegistryEntryID(now, &current_id) == KERN_SUCCESS && current_id == registry_id;
    if (now) IOObjectRelease(now);
    if (!same) rc = 1;
done:
    if (description) CFRelease(description);
    if (disk) CFRelease(disk);
    if (protocol) CFRelease(protocol);
    if (characteristics) CFRelease(characteristics);
    CFRelease(props);
#undef TRY
    return rc;
}

/** @brief Enumerate native media or resolve a block selector, preserving APFS role distinctions. */
int ds_discover(ds_inventory *inventory, const char *selector, char *error, size_t size)
{
    struct stat selected = {0};
    if (selector && stat(selector, &selected) < 0) {
        int saved = errno;
        (void)snprintf(error, size, "Cannot resolve target device.");
        return saved == EACCES || saved == EPERM ? 4 : 3;
    }
    if (selector && !S_ISBLK(selected.st_mode)) {
        (void)snprintf(error, size, "Target is not a block device; use /dev/diskN (not a raw character node).");
        return 3;
    }
    DASessionRef session = DASessionCreate(kCFAllocatorDefault);
    if (!session) { (void)snprintf(error, size, "Cannot initialize Disk Arbitration; check sandbox permissions."); return 1; }
    io_iterator_t iterator = IO_OBJECT_NULL;
    kern_return_t status = IOServiceGetMatchingServices(kIOMainPortDefault, IOServiceMatching("IOMedia"), &iterator);
    if (status != KERN_SUCCESS) { CFRelease(session); (void)snprintf(error, size, "Cannot enumerate IOKit media."); return 1; }
    int rc = 0;
    io_service_t media;
    while ((media = IOIteratorNext(iterator)) != IO_OBJECT_NULL) {
        ds_device d;
        ds_device_init(&d);
        int result = collect(media, session, &d);
        IOObjectRelease(media);
        if (result == 1) result = ds_strings_add(&inventory->warnings, "device_record_unavailable_or_changed");
        else if (result == 0 && (!selector || (d.major_number == (unsigned int)major(selected.st_rdev) && d.minor_number == (unsigned int)minor(selected.st_rdev))))
            result = ds_inventory_add(inventory, &d);
        ds_device_free(&d);
        if (result < 0) { rc = 1; (void)snprintf(error, size, "Memory allocation failed."); break; }
    }
    if (rc == 0 && !IOIteratorIsValid(iterator)) { rc = 1; (void)snprintf(error, size, "IOKit inventory changed during enumeration; retry."); }
    IOObjectRelease(iterator);
    CFRelease(session);
    if (rc == 0 && selector) {
        struct stat after;
        if (inventory->count != 1U || stat(selector, &after) < 0 || !S_ISBLK(after.st_mode) || after.st_rdev != selected.st_rdev) {
            rc = 3; (void)snprintf(error, size, "Target disappeared or changed identity.");
        }
    } else if (rc == 0 && !inventory->count && inventory->warnings.count) {
        rc = 1; (void)snprintf(error, size, "All media records became unavailable.");
    }
    return rc;
}
