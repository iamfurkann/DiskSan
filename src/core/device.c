/**
 * @file
 * @brief Own device metadata, preserve evidence, and manage inventory identity.
 */
#include "disksan/device.h"
#include <stdlib.h>
#include <inttypes.h>
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

/** @brief Describe a field by its offset without assuming padding between struct members. */
typedef struct { size_t offset; const char *name; } field_spec;
/** @brief List all owned text fields for cloning and provenance. */
static const field_spec text_fields[] = {
/** @brief Associate a text member offset with its public name. */
#define TEXT(name) {offsetof(ds_device, name), #name}
    TEXT(path), TEXT(sysfs_path), TEXT(model), TEXT(vendor), TEXT(serial), TEXT(wwn),
    TEXT(eui), TEXT(nguid), TEXT(parent), TEXT(controller), TEXT(device_id)
#undef TEXT
};
/** @brief List numeric fields with their serialized names. */
static const field_spec number_fields[] = {
/** @brief Associate a numeric member offset with its public name. */
#define NUMBER(name) {offsetof(ds_device, name), #name}
    NUMBER(size_bytes), NUMBER(logical_sector_size), NUMBER(physical_sector_size),
    NUMBER(rotational), NUMBER(removable), NUMBER(read_only)
#undef NUMBER
};
/** @brief Copy bounded text for supplemental evidence, returning null on failure. */
static char *copy_text(const char *text)
{
    size_t length = strlen(text);
    if (length > DS_TEXT_LIMIT) return NULL;
    char *copy = malloc(length + 1U);
    if (copy) memcpy(copy, text, length + 1U);
    return copy;
}
/** @brief Add one deduplicated observation with a strict per-record memory bound. */
static int observe(ds_device *d, const char *field, const char *source, const char *value)
{
    if (!source) source = "unknown";
    for (size_t i = 0; i < d->evidence_count; ++i) {
        const ds_observation *o = &d->evidence[i];
        if (!strcmp(o->field, field) && !strcmp(o->source, source) && !strcmp(o->value, value)) return 0;
    }
    if (d->evidence_count == DS_EVIDENCE_LIMIT) return ds_warn(d, "evidence_limit_reached");
    ds_observation o = {copy_text(field), copy_text(source), copy_text(value)};
    ds_observation *items = NULL;
    if (o.field && o.source && o.value)
        items = realloc(d->evidence, (d->evidence_count + 1U) * sizeof(*items));
    if (!items) { free(o.field); free(o.source); free(o.value); return -1; }
    d->evidence = items;
    items[d->evidence_count++] = o;
    return 0;
}
/** @brief Find the public field name for a metadata address belonging to this record. */
static const char *field_name(const ds_device *d, const void *field, const field_spec *fields, size_t count)
{
    for (size_t i = 0; i < count; ++i)
        if ((const char *)d + fields[i].offset == (const char *)field) return fields[i].name;
    return "external";
}

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
        bool differs = strlen(field->value) != len || memcmp(field->value, value, len) != 0;
        if (differs && ds_warn(d, "metadata_conflict") < 0) return -1;
        if (!differs && ((!field->source && !source) || (field->source && source && !strcmp(field->source, source)))) return 0;
        char normalized[DS_TEXT_LIMIT + 1U];
        memcpy(normalized, value, len); normalized[len] = '\0';
        const char *name = field_name(d, field, text_fields, sizeof(text_fields) / sizeof(*text_fields));
        if (observe(d, name, field->source, field->value) < 0) return -1;
        return observe(d, name, source, normalized);
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
    if (field->known) {
        if (field->value != value && ds_warn(d, "metadata_conflict") < 0) return -1;
        if (field->value == value && ((!field->source && !source) || (field->source && source && !strcmp(field->source, source)))) return 0;
        char before[32], after[32];
        (void)snprintf(before, sizeof(before), "%" PRIu64, field->value);
        (void)snprintf(after, sizeof(after), "%" PRIu64, value);
        const char *name = field_name(d, field, number_fields, sizeof(number_fields) / sizeof(*number_fields));
        if (observe(d, name, field->source, before) < 0) return -1;
        return observe(d, name, source, after);
    }
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
    for (size_t i = 0; i < d->evidence_count; ++i) {
        free(d->evidence[i].field); free(d->evidence[i].source); free(d->evidence[i].value);
    }
    free(d->evidence);
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

/** @brief Copy or combine metadata into a temporary record; caller releases it on failure. */
static int combine(ds_device *dst, const ds_device *src)
{
    for (size_t i = 0; i < sizeof(text_fields) / sizeof(*text_fields); ++i) {
        const ds_text *from = (const ds_text *)(const void *)((const char *)src + text_fields[i].offset);
        ds_text *to = (ds_text *)(void *)((char *)dst + text_fields[i].offset);
        if (i == 0U && to->value && from->value && strcmp(to->value, from->value)) {
            if (ds_strings_add(&dst->stable_paths, to->value) < 0 || ds_strings_add(&dst->stable_paths, from->value) < 0 ||
                observe(dst, "path", from->source, from->value) < 0) return -1;
        } else if (ds_text_offer(dst, to, from->value, from->source) < 0) return -1;
    }
    for (size_t i = 0; i < sizeof(number_fields) / sizeof(*number_fields); ++i) {
        const ds_number *from = (const ds_number *)(const void *)((const char *)src + number_fields[i].offset);
        ds_number *to = (ds_number *)(void *)((char *)dst + number_fields[i].offset);
        if (from->known && ds_number_offer(dst, to, from->value, from->source) < 0) return -1;
    }
    ds_strings *to[] = {&dst->stable_paths, &dst->holders, &dst->slaves, &dst->warnings};
    const ds_strings *from[] = {&src->stable_paths, &src->holders, &src->slaves, &src->warnings};
    for (size_t i = 0; i < sizeof(to) / sizeof(*to); ++i)
        for (size_t j = 0; j < from[i]->count; ++j)
            if (ds_strings_add(to[i], from[i]->items[j]) < 0) return -1;
    for (size_t i = 0; i < src->evidence_count; ++i) {
        const ds_observation *o = &src->evidence[i];
        if (observe(dst, o->field, o->source, o->value) < 0) return -1;
    }
    return 0;
}
/** @brief Merge equal native identities transactionally; reject ambiguous collisions unchanged. */
int ds_inventory_add(ds_inventory *inventory, ds_device *device)
{
    size_t match = SIZE_MAX;
    for (size_t i = 0; i < inventory->count; ++i) {
        const ds_device *existing = &inventory->items[i];
        if (strcmp(existing->platform, device->platform)) continue;
        bool both_numbers = existing->has_devnum && device->has_devnum;
        bool same_number = both_numbers && existing->major_number == device->major_number && existing->minor_number == device->minor_number;
        bool same_id = existing->device_id.value && device->device_id.value && !strcmp(existing->device_id.value, device->device_id.value);
        bool same_path = existing->path.value && device->path.value && !strcmp(existing->path.value, device->path.value);
        if (!same_number && !same_id && !same_path) continue;
        if (!same_id || !strcmp(device->platform, "unknown") ||
            (both_numbers && !same_number) || strcmp(existing->role, device->role) || match != SIZE_MAX) return -2;
        match = i;
    }
    if (match != SIZE_MAX) {
        ds_device merged;
        ds_device_init(&merged);
        const ds_device *existing = &inventory->items[match];
        merged.platform = existing->platform; merged.role = existing->role;
        merged.transport = existing->transport; merged.protocol = existing->protocol;
        merged.media = existing->media; merged.media_confidence = existing->media_confidence;
        merged.classification_source = existing->classification_source;
        merged.has_devnum = existing->has_devnum || device->has_devnum;
        merged.major_number = existing->has_devnum ? existing->major_number : device->major_number;
        merged.minor_number = existing->has_devnum ? existing->minor_number : device->minor_number;
        if (combine(&merged, existing) < 0 || combine(&merged, device) < 0 || ds_warn(&merged, "duplicate_record_merged") < 0)
            goto failed;
        if (existing->has_devnum != device->has_devnum && ds_warn(&merged, "identity_partially_verified") < 0) goto failed;
        if (strcmp(existing->transport, device->transport) || strcmp(existing->protocol, device->protocol) || strcmp(existing->media, device->media)) {
            merged.transport = merged.protocol = merged.media = merged.media_confidence = "unknown";
            if (ds_warn(&merged, "classification_conflict") < 0) goto failed;
        }
        ds_device_free(&inventory->items[match]);
        inventory->items[match] = merged;
        ds_device_free(device);
        return 0;
failed:
        ds_device_free(&merged);
        return -1;
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
    int path_order = strcmp(x->path.value ? x->path.value : x->sysfs_path.value ? x->sysfs_path.value : "",
                  y->path.value ? y->path.value : y->sysfs_path.value ? y->sysfs_path.value : "");
    if (path_order) return path_order;
    int platform_order = strcmp(x->platform, y->platform);
    if (platform_order) return platform_order;
    return strcmp(x->device_id.value ? x->device_id.value : "", y->device_id.value ? y->device_id.value : "");
}
/** @brief Sort observations independently of allocation order. */
static int compare_observations(const void *a, const void *b)
{
    const ds_observation *x = a, *y = b;
    int order = strcmp(x->field, y->field);
    if (!order) order = strcmp(x->source, y->source);
    return order ? order : strcmp(x->value, y->value);
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
        if (d->evidence_count > 1U) qsort(d->evidence, d->evidence_count, sizeof(*d->evidence), compare_observations);
    }
    ds_strings_sort(&inventory->warnings);
}

/** @brief Include every role except partitions, volumes, and snapshots in the default list. */
bool ds_default_visible(const ds_device *device)
{
    return strcmp(device->role, "partition") != 0 && strcmp(device->role, "volume") != 0 &&
           strcmp(device->role, "snapshot") != 0;
}
