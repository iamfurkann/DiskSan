/**
 * @file
 * @brief Discover disk interfaces through SetupAPI and read-only Windows storage queries.
 */
#include "disksan/discovery.h"
#include <windows.h>
#include <setupapi.h>
#include <winioctl.h>
#include <initguid.h>
#include <ntddstor.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** @brief Bound Windows descriptor and interface allocations to one mebibyte. */
#define DS_NATIVE_LIMIT (1024U * 1024U)

/** @brief Allocate bounded UTF-8 text from UTF-16; the caller frees the result, which may be null. */
static char *utf8(const wchar_t *wide)
{
    int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide, -1, NULL, 0, NULL, NULL);
    if (size <= 0 || size > (int)DS_TEXT_LIMIT + 1) return NULL;
    char *text = malloc((size_t)size);
    if (!text) return NULL;
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide, -1, text, size, NULL, NULL)) { free(text); return NULL; }
    return text;
}

/** @brief Translate a saved Windows query error into a deduplicated device warning. */
static int query_warning(ds_device *d, DWORD error)
{
    if (error == ERROR_ACCESS_DENIED) return ds_warn(d, "device_query_permission_denied");
    if (error == ERROR_INVALID_FUNCTION || error == ERROR_NOT_SUPPORTED) return ds_warn(d, "device_query_unsupported");
    return ds_warn(d, "device_query_failed");
}

/** @brief Validate a descriptor offset and terminator before offering the referenced string. */
static int descriptor_text(ds_device *d, ds_text *field, const BYTE *buffer, size_t length, DWORD offset, const char *source)
{
    if (!offset) return 0;
    if ((size_t)offset >= length || !memchr(buffer + offset, '\0', length - offset))
        return ds_warn(d, "invalid_native_metadata");
    return ds_text_offer(d, field, (const char *)(buffer + offset), source);
}

/** @brief Query a bounded storage descriptor, derive bus facts, and release the temporary buffer. */
static int properties(ds_device *d, HANDLE handle, ds_facts *facts)
{
    STORAGE_PROPERTY_QUERY query = {0};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    STORAGE_DESCRIPTOR_HEADER header = {0};
    DWORD returned = 0;
    if (!DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &header, sizeof(header), &returned, NULL))
        return query_warning(d, GetLastError());
    if (returned < sizeof(header) || header.Size < sizeof(STORAGE_DEVICE_DESCRIPTOR) || header.Size > DS_NATIVE_LIMIT)
        return ds_warn(d, "invalid_native_metadata");
    BYTE *buffer = calloc(1, header.Size);
    if (!buffer) return -1;
    int rc = 0;
    if (!DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), buffer, header.Size, &returned, NULL)) {
        DWORD saved = GetLastError();
        free(buffer);
        return query_warning(d, saved);
    }
    STORAGE_DEVICE_DESCRIPTOR *descriptor = (STORAGE_DEVICE_DESCRIPTOR *)(void *)buffer;
    if (returned < sizeof(*descriptor) || returned > header.Size || descriptor->Size > returned || descriptor->Size < sizeof(*descriptor)) {
        free(buffer);
        return ds_warn(d, "invalid_native_metadata");
    }
/** @brief Propagate metadata allocation failure through the local cleanup policy. */
#define TRY(expr) do { if ((expr) < 0) { rc = -1; goto done; } } while (0)
    TRY(descriptor_text(d, &d->model, buffer, descriptor->Size, descriptor->ProductIdOffset, "windows:ProductId"));
    TRY(descriptor_text(d, &d->vendor, buffer, descriptor->Size, descriptor->VendorIdOffset, "windows:VendorId"));
    TRY(descriptor_text(d, &d->serial, buffer, descriptor->Size, descriptor->SerialNumberOffset, "windows:SerialNumber"));
    TRY(ds_number_offer(d, &d->removable, descriptor->RemovableMedia ? 1U : 0U, "windows:RemovableMedia"));
    facts->optical = descriptor->DeviceType == 5U;
    switch (descriptor->BusType) {
        case BusTypeUsb: facts->usb = true; break;
        case BusTypeAta: case BusTypeSata: facts->ata = true; break;
        case BusTypeScsi: case BusTypeSas: facts->scsi = true; break;
        case BusTypeNvme: facts->nvme = true; break;
        case BusTypeVirtual: case BusTypeFileBackedVirtual: facts->virtual_device = true; break;
        case BusTypeRAID: case BusTypeSpaces: facts->composite = true; break;
        case BusTypeMmc: case BusTypeSd: facts->mmc = true; break;
        default: break;
    }
done:
    free(buffer);
#undef TRY
    return rc;
}

/** @brief Collect an interface using read-only or zero-access handles and recheck device identity. */
static int collect(const wchar_t *interface_path, bool partition, ds_device *d)
{
    char *path = utf8(interface_path);
    if (!path) return -1;
    d->platform = "windows";
    int rc = ds_text_offer(d, &d->device_id, path, "setupapi:device_interface");
    if (rc == 0) rc = ds_text_offer(d, &d->path, path, "setupapi:device_interface");
    if (rc == 0) rc = ds_strings_add(&d->stable_paths, path);
    free(path);
    if (rc < 0) return -1;
    ds_facts facts = {.disk = !partition, .partition = partition};
    /* Sharing allows existing OS users; desired access NEVER includes write. */
    HANDLE handle = CreateFileW(interface_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL, OPEN_EXISTING, 0, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        DWORD saved = GetLastError();
        if (saved == ERROR_ACCESS_DENIED) {
            if (ds_warn(d, "device_read_permission_denied") < 0) return -1;
            handle = CreateFileW(interface_path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        }
        if (handle == INVALID_HANDLE_VALUE) {
            if (ds_classify(d, &facts) < 0) return -1;
            d->classification_source = "windows_setupapi";
            return ds_warn(d, "device_metadata_unavailable");
        }
    }
/** @brief Propagate metadata allocation failure through the local cleanup policy. */
#define TRY(expr) do { if ((expr) < 0) { rc = -1; goto done; } } while (0)
    STORAGE_DEVICE_NUMBER identity = {0};
    DWORD returned = 0;
    bool has_identity = DeviceIoControl(handle, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &identity, sizeof(identity), &returned, NULL) && returned >= sizeof(identity);
    if (has_identity) {
        char canonical[80];
        (void)snprintf(canonical, sizeof(canonical), "\\\\.\\PhysicalDrive%lu", (unsigned long)identity.DeviceNumber);
        if (partition) TRY(ds_text_offer(d, &d->parent, canonical, "windows:DeviceNumber"));
        else {
            free(d->path.value); d->path.value = NULL; d->path.source = NULL;
            TRY(ds_text_offer(d, &d->path, canonical, "windows:DeviceNumber"));
        }
    } else TRY(ds_warn(d, "device_number_unavailable"));
    TRY(properties(d, handle, &facts));
    GET_LENGTH_INFORMATION length = {0};
    if (DeviceIoControl(handle, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &length, sizeof(length), &returned, NULL) &&
        returned >= sizeof(length) && length.Length.QuadPart >= 0)
        TRY(ds_number_offer(d, &d->size_bytes, (uint64_t)length.Length.QuadPart, "windows:IOCTL_DISK_GET_LENGTH_INFO"));
    else TRY(ds_warn(d, "capacity_unavailable"));
    STORAGE_PROPERTY_QUERY query = {0};
    query.PropertyId = StorageAccessAlignmentProperty;
    query.QueryType = PropertyStandardQuery;
    STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR alignment = {0};
    if (DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &alignment, sizeof(alignment), &returned, NULL) && returned >= sizeof(alignment)) {
        if (alignment.BytesPerLogicalSector) TRY(ds_number_offer(d, &d->logical_sector_size, alignment.BytesPerLogicalSector, "windows:BytesPerLogicalSector"));
        if (alignment.BytesPerPhysicalSector) TRY(ds_number_offer(d, &d->physical_sector_size, alignment.BytesPerPhysicalSector, "windows:BytesPerPhysicalSector"));
    }
    if (!d->logical_sector_size.known || !d->physical_sector_size.known) TRY(ds_warn(d, "sector_size_unavailable"));
    /* This control code queries writability; it performs no write operation. */
    if (DeviceIoControl(handle, IOCTL_DISK_IS_WRITABLE, NULL, 0, NULL, 0, &returned, NULL))
        TRY(ds_number_offer(d, &d->read_only, 0, "windows:IOCTL_DISK_IS_WRITABLE"));
    else {
        DWORD saved = GetLastError();
        if (saved == ERROR_WRITE_PROTECT) TRY(ds_number_offer(d, &d->read_only, 1, "windows:IOCTL_DISK_IS_WRITABLE"));
        else TRY(ds_warn(d, "read_only_flag_unavailable"));
    }
    TRY(ds_classify(d, &facts));
    d->classification_source = "windows_storage_properties";
    TRY(ds_warn(d, d->serial.value ? "identity_uniqueness_unverified" : "persistent_identity_unavailable"));
    if (has_identity) {
        STORAGE_DEVICE_NUMBER after = {0};
        if (!DeviceIoControl(handle, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &after, sizeof(after), &returned, NULL) ||
            returned < sizeof(after) || after.DeviceType != identity.DeviceType || after.DeviceNumber != identity.DeviceNumber || after.PartitionNumber != identity.PartitionNumber)
            rc = 1;
    }
done:
    CloseHandle(handle);
#undef TRY
    return rc;
}

/** @brief Match an optional selector against discovered native paths without opening user input. */
static bool selected(const ds_device *d, const char *selector)
{
    if (!selector) return true;
    return (d->path.value && _stricmp(selector, d->path.value) == 0) ||
           (d->device_id.value && _stricmp(selector, d->device_id.value) == 0);
}

/** @brief Enumerate present disk and partition interfaces and reject unmatched selectors. */
int ds_discover(ds_inventory *inventory, const char *selector, char *error, size_t size)
{
    const GUID *classes[] = {&GUID_DEVINTERFACE_DISK, &GUID_DEVINTERFACE_PARTITION};
    for (size_t type = 0; type < 2U; ++type) {
        HDEVINFO set = SetupDiGetClassDevsW(classes[type], NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (set == INVALID_HANDLE_VALUE) { (void)snprintf(error, size, "Cannot initialize Windows device enumeration."); return 1; }
        int rc = 0;
        for (DWORD index = 0; ; ++index) {
            SP_DEVICE_INTERFACE_DATA data = {0};
            data.cbSize = sizeof(data);
            if (!SetupDiEnumDeviceInterfaces(set, NULL, classes[type], index, &data)) {
                DWORD saved = GetLastError();
                if (saved != ERROR_NO_MORE_ITEMS) rc = 1;
                break;
            }
            DWORD needed = 0;
            (void)SetupDiGetDeviceInterfaceDetailW(set, &data, NULL, 0, &needed, NULL);
            if (needed < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) || needed > DS_NATIVE_LIMIT) { rc = 1; break; }
            SP_DEVICE_INTERFACE_DETAIL_DATA_W *detail = calloc(1, needed);
            if (!detail) { rc = 1; break; }
            detail->cbSize = sizeof(*detail);
            if (!SetupDiGetDeviceInterfaceDetailW(set, &data, detail, needed, NULL, NULL)) {
                free(detail);
                if (ds_strings_add(&inventory->warnings, "device_record_unavailable_or_changed") < 0) { rc = 1; break; }
                continue;
            }
            ds_device d;
            ds_device_init(&d);
            int result = collect(detail->DevicePath, type == 1U, &d);
            free(detail);
            if (result == 1) result = ds_strings_add(&inventory->warnings, "device_record_unavailable_or_changed");
            else if (result == 0 && selected(&d, selector)) result = ds_inventory_add(inventory, &d);
            ds_device_free(&d);
            if (result < 0) { rc = 1; break; }
        }
        SetupDiDestroyDeviceInfoList(set);
        if (rc) { (void)snprintf(error, size, "Windows device enumeration failed or allocation was unavailable."); return rc; }
    }
    if (selector && !inventory->count) { (void)snprintf(error, size, "Target is not a present disk/partition device interface."); return 3; }
    if (!inventory->count && inventory->warnings.count) { (void)snprintf(error, size, "All device records became unavailable."); return 1; }
    return 0;
}
