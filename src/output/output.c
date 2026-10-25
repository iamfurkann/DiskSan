/**
 * @file
 * @brief Serialize inventory snapshots as safe terminal text or schema 2 JSON.
 */
#include "disksan/device.h"
#include "disksan/version.h"
#include <inttypes.h>
#include <string.h>

/* Reject overlong UTF-8, surrogates and code points beyond U+10FFFF. */
/** @brief Return the valid UTF-8 sequence length, or zero for malformed or incomplete input. */
static size_t utf8_sequence(const unsigned char *s, size_t available)
{
    unsigned int cp;
    size_t count;
    if (*s < 0x80U) return 1;
    if (*s >= 0xc2U && *s <= 0xdfU) { count = 2; cp = *s & 0x1fU; }
    else if (*s >= 0xe0U && *s <= 0xefU) { count = 3; cp = *s & 0x0fU; }
    else if (*s >= 0xf0U && *s <= 0xf4U) { count = 4; cp = *s & 0x07U; }
    else return 0;
    if (count > available) return 0;
    for (size_t i = 1; i < count; ++i) {
        if (s[i] == 0 || (s[i] & 0xc0U) != 0x80U) return 0;
        cp = (cp << 6U) | (s[i] & 0x3fU);
    }
    if ((count == 2 && cp < 0x80U) || (count == 3 && cp < 0x800U) ||
        (count == 4 && cp < 0x10000U) || cp > 0x10ffffU ||
        (cp >= 0xd800U && cp <= 0xdfffU)) return 0;
    return count;
}

/** @brief Write a JSON string or null, escaping controls and replacing malformed UTF-8 bytes. */
void ds_json_string(FILE *out, const char *text)
{
    if (!text) { fputs("null", out); return; }
    fputc('"', out);
    const unsigned char *s = (const unsigned char *)text;
    size_t remaining = strlen(text);
    while (remaining) {
        size_t consumed = 1;
        if (*s == '"' || *s == '\\') { fputc('\\', out); fputc(*s, out); }
        else if (*s < 0x20U || *s == 0x7fU) { fprintf(out, "\\u%04x", (unsigned int)*s); }
        else if (*s < 0x80U) fputc(*s, out);
        else {
            size_t n = utf8_sequence(s, remaining);
            if (!n || n > remaining) { fputs("\\ufffd", out); }
            else if (n == 2U && s[0] == 0xc2U && s[1] <= 0x9fU) {
                fprintf(out, "\\u%04x", (unsigned int)s[1]);
                consumed = n;
            } else { (void)fwrite(s, 1, n, out); consumed = n; }
        }
        s += consumed;
        remaining -= consumed;
    }
    fputc('"', out);
}

/** @brief Write bounded ASCII-safe text, escaping non-ASCII bytes and control characters. */
void ds_terminal_string(FILE *out, const char *text, size_t limit)
{
    if (!text) { fputs("unknown", out); return; }
    const unsigned char *s = (const unsigned char *)text;
    size_t width = 0;
    while (*s) {
        size_t n = (*s < 0x20U || *s >= 0x7fU || *s == '\\') ? 4U : 1U;
        if (width > limit || n > limit - width) { fputs("...", out); break; }
        if (n == 4U) fprintf(out, "\\x%02x", (unsigned int)*s);
        else fputc(*s, out);
        ++s;
        width += n;
    }
}

/** @brief Serialize a known integer or boolean, retaining null for unknown metadata. */
static void json_number(FILE *out, const ds_number *n, bool boolean)
{
    if (!n->known) fputs("null", out);
    else if (boolean) fputs(n->value ? "true" : "false", out);
    else fprintf(out, "%" PRIu64, n->value);
}

/** @brief Serialize an owned string list as a JSON array with safe escaping. */
static void json_strings(FILE *out, const ds_strings *strings)
{
    fputc('[', out);
    for (size_t i = 0; i < strings->count; ++i) {
        if (i) fputc(',', out);
        ds_json_string(out, strings->items[i]);
    }
    fputc(']', out);
}

/** @brief Serialize all device fields, evidence sources, relationships, and warnings. */
static void json_device(FILE *out, const ds_device *d)
{
    const char *text_names[] = {"path", "sysfs_path", "model", "vendor", "serial", "wwn", "eui", "nguid", "parent", "controller", "device_id"};
    const ds_text *texts[] = {&d->path, &d->sysfs_path, &d->model, &d->vendor, &d->serial, &d->wwn, &d->eui, &d->nguid, &d->parent, &d->controller, &d->device_id};
    const char *number_names[] = {"size_bytes", "logical_sector_size", "physical_sector_size", "rotational", "removable", "read_only"};
    const ds_number *numbers[] = {&d->size_bytes, &d->logical_sector_size, &d->physical_sector_size, &d->rotational, &d->removable, &d->read_only};
    fputc('{', out);
    for (size_t i = 0; i < sizeof(texts) / sizeof(*texts); ++i) {
        if (i) fputc(',', out);
        ds_json_string(out, text_names[i]); fputc(':', out); ds_json_string(out, texts[i]->value);
    }
    if (d->has_devnum) fprintf(out, ",\"major\":%u,\"minor\":%u", d->major_number, d->minor_number);
    else fputs(",\"major\":null,\"minor\":null", out);
    fputs(",\"platform\":", out); ds_json_string(out, d->platform);
    for (size_t i = 0; i < sizeof(numbers) / sizeof(*numbers); ++i) {
        fputc(',', out); ds_json_string(out, number_names[i]); fputc(':', out);
        json_number(out, numbers[i], i >= 3U);
    }
    const char *enum_names[] = {"device_role", "transport", "protocol", "media_type", "media_confidence", "classification_source"};
    const char *enums[] = {d->role, d->transport, d->protocol, d->media, d->media_confidence, d->classification_source};
    for (size_t i = 0; i < sizeof(enums) / sizeof(*enums); ++i) {
        fputc(',', out); ds_json_string(out, enum_names[i]); fputc(':', out); ds_json_string(out, enums[i]);
    }
    fputs(",\"sources\":{", out);
    for (size_t i = 0; i < sizeof(texts) / sizeof(*texts); ++i) {
        if (i) fputc(',', out);
        ds_json_string(out, text_names[i]); fputc(':', out); ds_json_string(out, texts[i]->source);
    }
    for (size_t i = 0; i < sizeof(numbers) / sizeof(*numbers); ++i) {
        fputc(',', out); ds_json_string(out, number_names[i]); fputc(':', out); ds_json_string(out, numbers[i]->source);
    }
    fputs("},\"stable_paths\":", out); json_strings(out, &d->stable_paths);
    fputs(",\"holders\":", out); json_strings(out, &d->holders);
    fputs(",\"slaves\":", out); json_strings(out, &d->slaves);
    fputs(",\"evidence\":[", out);
    for (size_t i = 0; i < d->evidence_count; ++i) {
        const ds_observation *o = &d->evidence[i];
        if (i) fputc(',', out);
        fputs("{\"field\":", out); ds_json_string(out, o->field);
        fputs(",\"source\":", out); ds_json_string(out, o->source);
        fputs(",\"value\":", out); ds_json_string(out, o->value);
        fputc('}', out);
    }
    fputs("],\"warnings\":", out); json_strings(out, &d->warnings);
    fputc('}', out);
}

/** @brief Write one labeled text field through the terminal escaping policy. */
static void text_field(FILE *out, const char *name, const char *value)
{
    fprintf(out, "%-23s ", name);
    ds_terminal_string(out, value, DS_TEXT_LIMIT * 4U);
    fputc('\n', out);
}

/** @brief Write one labeled numeric field, preserving unknown values as null. */
static void text_number(FILE *out, const char *name, const ds_number *n, bool boolean)
{
    fprintf(out, "%-23s ", name);
    json_number(out, n, boolean);
    fputc('\n', out);
}

/** @brief Write a labeled list without allowing metadata to inject terminal controls. */
static void text_strings(FILE *out, const char *name, const ds_strings *strings)
{
    fprintf(out, "%-23s", name);
    if (!strings->count) fputs(" []", out);
    for (size_t i = 0; i < strings->count; ++i) {
        fputc(' ', out);
        ds_terminal_string(out, strings->items[i], DS_TEXT_LIMIT * 4U);
    }
    fputc('\n', out);
}

/** @brief Render the complete human-readable metadata for one device record. */
static void text_detail(FILE *out, const ds_device *d)
{
    text_field(out, "Device:", d->path.value);
    if (d->sysfs_path.value) text_field(out, "Sysfs:", d->sysfs_path.value);
    text_field(out, "Platform:", d->platform);
    text_field(out, "Native identity:", d->device_id.value);
    if (d->has_devnum) fprintf(out, "Device number:          %u:%u\n", d->major_number, d->minor_number);
    text_field(out, "Role:", d->role);
    text_field(out, "Transport:", d->transport);
    text_field(out, "Protocol:", d->protocol);
    text_field(out, "Media:", d->media);
    text_field(out, "Media confidence:", d->media_confidence);
    text_field(out, "Model:", d->model.value);
    text_field(out, "Vendor:", d->vendor.value);
    text_field(out, "Serial:", d->serial.value);
    text_field(out, "WWN:", d->wwn.value);
    text_field(out, "EUI:", d->eui.value);
    text_field(out, "NGUID:", d->nguid.value);
    text_field(out, "Parent:", d->parent.value);
    text_field(out, "Controller:", d->controller.value);
    text_number(out, "Capacity (bytes):", &d->size_bytes, false);
    text_number(out, "Logical sector (bytes):", &d->logical_sector_size, false);
    text_number(out, "Physical sector:", &d->physical_sector_size, false);
    text_number(out, "Rotational:", &d->rotational, true);
    text_number(out, "Removable:", &d->removable, true);
    text_number(out, "Device read-only:", &d->read_only, true);
    text_strings(out, "Stable paths:", &d->stable_paths);
    text_strings(out, "Holders:", &d->holders);
    text_strings(out, "Slaves:", &d->slaves);
    text_strings(out, "Warnings:", &d->warnings);
    for (size_t i = 0; i < d->evidence_count; ++i) {
        text_field(out, "Evidence field:", d->evidence[i].field);
        text_field(out, "Evidence source:", d->evidence[i].source);
        text_field(out, "Evidence value:", d->evidence[i].value);
    }
}

/** @brief Filter and serialize a snapshot, then report stream or flush failures. */
int ds_output(FILE *out, const ds_inventory *inventory, bool json, bool all, bool detail)
{
    size_t shown = 0;
    if (json) {
        fputs("{\"schema_version\":2,\"application\":\"DiskSan\",\"version\":", out);
        ds_json_string(out, DISKSAN_VERSION);
        fputs(",\"mode\":\"read-only\",\"snapshot\":true,\"devices\":[", out);
    } else {
        fprintf(out, "DiskSan v%s - Device Discovery (read-only snapshot)\n", DISKSAN_VERSION);
        if (!detail) fputs("DEVICE | BYTES | ROLE | TRANSPORT | PROTOCOL | MEDIA | MODEL\n", out);
    }
    for (size_t i = 0; i < inventory->count; ++i) {
        const ds_device *d = &inventory->items[i];
        if (!all && !ds_default_visible(d)) continue;
        if (json) {
            if (shown) fputc(',', out);
            json_device(out, d);
        } else if (detail) text_detail(out, d);
        else {
            ds_terminal_string(out, d->path.value, 48);
            fputs(" | ", out); json_number(out, &d->size_bytes, false);
            fprintf(out, " | %s | %s | %s | %s | ", d->role, d->transport, d->protocol, d->media);
            ds_terminal_string(out, d->model.value, 48);
            fputc('\n', out);
            if (d->warnings.count) text_strings(out, "  Warnings:", &d->warnings);
        }
        ++shown;
    }
    if (json) {
        fprintf(out, "],\"shown\":%zu,\"omitted\":%zu,\"warnings\":", shown, inventory->count - shown);
        json_strings(out, &inventory->warnings);
        fputs("}\n", out);
    } else {
        text_strings(out, "Snapshot warnings:", &inventory->warnings);
        fprintf(out, "%zu device record(s) shown; %zu omitted. This is not a physical disk count.\n", shown, inventory->count - shown);
        fputs("Operation mode: READ ONLY. Sanitize capability: NOT EVALUATED.\n", out);
    }
    int flush_result = fflush(out);
    return flush_result == 0 && !ferror(out) ? 0 : 1;
}
