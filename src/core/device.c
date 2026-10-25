/**
 * @file
 * @brief Own device metadata, preserve evidence, and manage inventory identity.
 */
#include "disksan/device.h"
#include <stdlib.h>
#include <string.h>

/** @brief Initialize an empty record with unknown classifications; release owned data before reinitializing. */
void ds_device_init(ds_device *d)
{
    memset(d, 0, sizeof(*d));
    d->role = d->transport = d->protocol = d->media = "unknown";
    d->media_confidence = "unknown";
    d->classification_source = "insufficient_evidence";
    d->platform = "unknown";
}

/** @brief Release every owned string and reset the list to an empty state. */
void ds_strings_free(ds_strings *list)
{
    for (size_t i = 0; i < list->count; ++i) free(list->items[i]);
    free(list->items);
    memset(list, 0, sizeof(*list));
}

/** @brief Copy a bounded string into a deduplicated list; return -1 if allocation or size checks fail. */
int ds_strings_add(ds_strings *list, const char *value)
{
    if (!value) return 0;
    for (size_t i = 0; i < list->count; ++i)
        if (strcmp(list->items[i], value) == 0) return 0;
    size_t len = strlen(value);
    if (len > DS_TEXT_LIMIT || list->count >= SIZE_MAX / sizeof(char *) - 1U) return -1;
    char *copy = malloc(len + 1U);
    if (!copy) return -1;
    memcpy(copy, value, len + 1U);
    char **items = realloc(list->items, (list->count + 1U) * sizeof(*items));
    if (!items) { free(copy); return -1; }
    list->items = items;
    list->items[list->count++] = copy;
    return 0;
}

/** @brief Compare two string pointers lexicographically for deterministic sorting. */
static int compare_strings(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

/** @brief Sort a string list in place without allocating additional strings. */
void ds_strings_sort(ds_strings *list)
{
    if (list->count > 1U) qsort(list->items, list->count, sizeof(*list->items), compare_strings);
}

/** @brief Add a deduplicated warning code; return -1 if the warning cannot be retained. */
int ds_warn(ds_device *d, const char *code) { return ds_strings_add(&d->warnings, code); }

/** @brief Normalize bounded text, retain the first value, and warn about conflicting later evidence. */
int ds_text_offer(ds_device *d, ds_text *field, const char *value, const char *source)
{
    if (!value) return 0;
    size_t len = 0;
    while (len <= DS_TEXT_LIMIT && value[len]) ++len;
    if (len > DS_TEXT_LIMIT) return ds_warn(d, "metadata_too_long");
    while (len && (value[len - 1U] == ' ' || value[len - 1U] == '\n' || value[len - 1U] == '\r' || value[len - 1U] == '\t')) --len;
    while (len && (*value == ' ' || *value == '\t')) { ++value; --len; }
    if (!len) return 0;
    if (field->value) {
        if (strlen(field->value) != len || memcmp(field->value, value, len) != 0)
            return ds_warn(d, "metadata_conflict");
        return 0;
    }
    field->value = malloc(len + 1U);
    if (!field->value) return -1;
    memcpy(field->value, value, len);
    field->value[len] = '\0';
    field->source = source;
    return 0;
}

/** @brief Retain the first known number and report conflicting values without overwriting it. */
int ds_number_offer(ds_device *d, ds_number *field, uint64_t value, const char *source)
{
    if (field->known) return field->value == value ? 0 : ds_warn(d, "metadata_conflict");
    field->value = value;
    field->known = true;
    field->source = source;
    return 0;
}

/** @brief Parse unsigned decimal text with optional trailing whitespace and reject overflow. */
bool ds_parse_u64(const char *text, uint64_t *value)
{
    if (!text || !*text || !value) return false;
    uint64_t n = 0;
    const unsigned char *s = (const unsigned char *)text;
    if (*s < '0' || *s > '9') return false;
    while (*s >= '0' && *s <= '9') {
        uint64_t digit = (uint64_t)(*s - '0');
        if (n > (UINT64_MAX - digit) / 10U) return false;
        n = n * 10U + digit;
        ++s;
    }
    while (*s == '\n' || *s == '\r' || *s == ' ' || *s == '\t') ++s;
    if (*s) return false;
    *value = n;
    return true;
}

/** @brief Convert a decimal count of 512-byte sysfs sectors without overflowing uint64_t. */
bool ds_sectors_to_bytes(const char *text, uint64_t *value)
{
    uint64_t sectors;
    if (!value || !ds_parse_u64(text, &sectors) || sectors > UINT64_MAX / 512U) return false;
    *value = sectors * 512U;
    return true;
}

/** @brief Release all owned record data and leave the record initialized for reuse. */
void ds_device_free(ds_device *d)
{
    ds_text *fields[] = {&d->path, &d->sysfs_path, &d->model, &d->vendor, &d->serial,
                         &d->wwn, &d->eui, &d->nguid, &d->parent, &d->controller, &d->device_id};
    for (size_t i = 0; i < sizeof(fields) / sizeof(*fields); ++i) free(fields[i]->value);
    ds_strings_free(&d->stable_paths);
    ds_strings_free(&d->holders);
    ds_strings_free(&d->slaves);
    ds_strings_free(&d->warnings);
    ds_device_init(d);
}

/** @brief Release records and snapshot warnings, then reset the inventory. */
void ds_inventory_free(ds_inventory *inventory)
{
    for (size_t i = 0; i < inventory->count; ++i) ds_device_free(&inventory->items[i]);
    free(inventory->items);
    ds_strings_free(&inventory->warnings);
    memset(inventory, 0, sizeof(*inventory));
}

/** @brief Consume a record on success, deduplicating by native identity or valid device number. */
int ds_inventory_add(ds_inventory *inventory, ds_device *device)
{
    for (size_t i = 0; i < inventory->count; ++i) {
        const ds_device *existing = &inventory->items[i];
        bool same_platform = strcmp(existing->platform, device->platform) == 0;
        bool same_number = existing->has_devnum && device->has_devnum &&
                           existing->major_number == device->major_number && existing->minor_number == device->minor_number;
        bool same_id = existing->device_id.value && device->device_id.value &&
                       strcmp(existing->device_id.value, device->device_id.value) == 0;
        if (same_platform && (same_number || same_id)) {
            ds_device_free(device);
            return 0;
        }
    }
    if (inventory->count >= SIZE_MAX / sizeof(ds_device) - 1U) return -1;
    ds_device *items = realloc(inventory->items, (inventory->count + 1U) * sizeof(*items));
    if (!items) return -1;
    inventory->items = items;
    items[inventory->count++] = *device;
    ds_device_init(device);
    return 0;
}

/** @brief Compare canonical display paths, falling back to sysfs paths when absent. */
static int compare_devices(const void *a, const void *b)
{
    const ds_device *x = a, *y = b;
    return strcmp(x->path.value ? x->path.value : x->sysfs_path.value ? x->sysfs_path.value : "",
                  y->path.value ? y->path.value : y->sysfs_path.value ? y->sysfs_path.value : "");
}

/** @brief Sort records, relationship lists, and warnings for deterministic output. */
void ds_inventory_sort(ds_inventory *inventory)
{
    if (inventory->count > 1U)
        qsort(inventory->items, inventory->count, sizeof(*inventory->items), compare_devices);
    for (size_t i = 0; i < inventory->count; ++i) {
        ds_device *d = &inventory->items[i];
        ds_strings_sort(&d->stable_paths);
        ds_strings_sort(&d->holders);
        ds_strings_sort(&d->slaves);
        ds_strings_sort(&d->warnings);
    }
    ds_strings_sort(&inventory->warnings);
}

/** @brief Include every role except partitions, volumes, and snapshots in the default list. */
bool ds_default_visible(const ds_device *device)
{
    return strcmp(device->role, "partition") != 0 && strcmp(device->role, "volume") != 0 &&
           strcmp(device->role, "snapshot") != 0;
}
