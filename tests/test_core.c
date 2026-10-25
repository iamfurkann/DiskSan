/**
 * @file
 * @brief Exercise normalization, classification, ownership, and snapshot identity.
 */
#include "disksan/device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int checks;
/** @brief Count an assertion and terminate the test on failure. */
#define CHECK(expr) do { ++checks; if (!(expr)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); exit(1); } } while (0)
/** @brief Check whether a record contains an expected warning code. */
static bool has_warning(const ds_device *d, const char *value)
{
    for (size_t i = 0; i < d->warnings.count; ++i)
        if (strcmp(d->warnings.items[i], value) == 0) return true;
    return false;
}

/** @brief Verify unsigned parsing and sector conversion at empty, invalid, and overflow boundaries. */
static void numbers(void)
{
    uint64_t n = 99;
    CHECK(ds_parse_u64("0", &n) && n == 0);
    CHECK(ds_parse_u64("18446744073709551615", &n) && n == UINT64_MAX);
    CHECK(!ds_parse_u64("18446744073709551616", &n));
    CHECK(!ds_parse_u64("-1", &n));
    CHECK(!ds_parse_u64("+1", &n));
    CHECK(!ds_parse_u64(" 1", &n));
    CHECK(!ds_parse_u64("", &n));
    CHECK(!ds_parse_u64(NULL, &n));
    CHECK(!ds_parse_u64("1", NULL));
    CHECK(!ds_parse_u64("1junk", &n));
    CHECK(ds_parse_u64("4096\n", &n) && n == 4096);
    CHECK(ds_sectors_to_bytes("2", &n) && n == 1024);
    CHECK(ds_sectors_to_bytes("36028797018963967", &n) && n == UINT64_MAX - 511U);
    CHECK(!ds_sectors_to_bytes("36028797018963968", &n));
    CHECK(!ds_sectors_to_bytes("18446744073709551616", &n));
    CHECK(!ds_sectors_to_bytes("1", NULL));
}

/** @brief Verify source precedence, conflicts, length limits, warnings, and repeated cleanup. */
static void normalization(void)
{
    ds_device d;
    ds_device_init(&d);
    CHECK(ds_text_offer(&d, &d.model, NULL, "test") == 0 && !d.model.value);
    CHECK(ds_text_offer(&d, &d.model, " \t\n", "test") == 0 && !d.model.value);
    CHECK(ds_text_offer(&d, &d.model, "  Example disk \n", "sysfs") == 0);
    CHECK(strcmp(d.model.value, "Example disk") == 0);
    CHECK(strcmp(d.model.source, "sysfs") == 0);
    CHECK(ds_text_offer(&d, &d.model, "Example disk", "udev") == 0 && d.warnings.count == 0);
    CHECK(ds_text_offer(&d, &d.model, "Different", "udev") == 0);
    CHECK(has_warning(&d, "metadata_conflict"));
    CHECK(strcmp(d.model.value, "Example disk") == 0);
    char long_text[DS_TEXT_LIMIT + 2U];
    memset(long_text, 'X', sizeof(long_text));
    long_text[sizeof(long_text) - 1U] = '\0';
    CHECK(ds_text_offer(&d, &d.serial, long_text, "test") == 0 && !d.serial.value);
    CHECK(has_warning(&d, "metadata_too_long"));
    long_text[DS_TEXT_LIMIT] = '\0';
    CHECK(ds_text_offer(&d, &d.serial, long_text, "test") == 0);
    CHECK(strlen(d.serial.value) == DS_TEXT_LIMIT);
    CHECK(ds_number_offer(&d, &d.size_bytes, 4096, "ioctl") == 0);
    CHECK(ds_number_offer(&d, &d.size_bytes, 0, "sysfs") == 0);
    CHECK(d.size_bytes.known && d.size_bytes.value == 4096);
    CHECK(strcmp(d.size_bytes.source, "ioctl") == 0);
    CHECK(ds_warn(&d, "metadata_conflict") == 0 && d.warnings.count == 2);
    ds_device_free(&d);
    ds_device_free(&d); /* Cleanup remains safe on empty/partially built records. */
}

/** @brief Verify the role, bus, media, confidence, and visibility of 24 evidence fixtures. */
static void classifications(void)
{
    struct fixture {
        const char *name;
        ds_facts facts;
        int rotational;
        const char *role, *transport, *protocol, *media;
    } cases[] = {
        {"NVMe namespace", {.disk=true, .nvme=true}, 0, "namespace", "nvme", "nvme", "ssd"},
        {"NVMe rotational conflict", {.disk=true, .nvme=true}, 1, "namespace", "nvme", "nvme", "unknown"},
        {"NVMe missing queue", {.disk=true, .nvme=true}, -1, "namespace", "nvme", "nvme", "unknown"},
        {"SATA HDD", {.disk=true, .ata=true, .scsi=true}, 1, "disk", "ata", "ata", "hdd"},
        {"SATA SSD", {.disk=true, .ata=true}, 0, "disk", "ata", "ata", "ssd"},
        {"SCSI HDD", {.disk=true, .scsi=true}, 1, "disk", "scsi", "scsi", "hdd"},
        {"SCSI optical", {.disk=true, .scsi=true, .optical=true}, 0, "disk", "scsi", "scsi", "unknown"},
        {"USB unknown protocol", {.disk=true, .usb=true, .scsi=true}, 0, "disk", "usb", "unknown", "unknown"},
        {"USB ATA evidence", {.disk=true, .usb=true, .ata=true}, 0, "disk", "usb", "ata", "unknown"},
        {"USB native NVMe evidence", {.disk=true, .usb=true, .nvme=true}, 0, "disk", "usb", "nvme", "unknown"},
        {"virtio", {.disk=true, .virtio=true}, 0, "virtual", "virtio", "unknown", "unknown"},
        {"loop", {.disk=true, .virtual_device=true}, 0, "virtual", "unknown", "unknown", "unknown"},
        {"dm", {.disk=true, .virtual_device=true, .composite=true}, 0, "composite", "unknown", "unknown", "unknown"},
        {"md", {.disk=true, .composite=true}, 1, "composite", "unknown", "unknown", "unknown"},
        {"partition", {.partition=true, .nvme=true}, 0, "partition", "nvme", "nvme", "unknown"},
        {"virtual partition", {.partition=true, .virtio=true}, 0, "partition", "virtio", "unknown", "unknown"},
        {"generic disk", {.disk=true}, 0, "disk", "unknown", "unknown", "unknown"},
        {"unrecognized", {0}, -1, "unknown", "unknown", "unknown", "unknown"},
        {"MMC", {.disk=true, .mmc=true}, 0, "disk", "mmc", "unknown", "flash"},
        {"NVMe multipath", {.disk=true, .nvme=true, .composite=true}, 0, "composite", "nvme", "nvme", "unknown"},
        {"APFS container", {.disk=true, .composite=true}, 0, "composite", "unknown", "unknown", "unknown"},
        {"APFS volume", {.partition=true, .volume=true}, 0, "volume", "unknown", "unknown", "unknown"},
        {"APFS snapshot", {.partition=true, .volume=true, .snapshot=true}, 0, "snapshot", "unknown", "unknown", "unknown"},
        {"Native medium report", {.disk=true, .media_reported=true}, 0, "disk", "unknown", "unknown", "ssd"},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
        ds_device d;
        ds_device_init(&d);
        if (cases[i].rotational >= 0)
            CHECK(ds_number_offer(&d, &d.rotational, (uint64_t)cases[i].rotational, "fixture:rotational") == 0);
        CHECK(ds_classify(&d, &cases[i].facts) == 0);
        CHECK(strcmp(d.role, cases[i].role) == 0);
        CHECK(strcmp(d.transport, cases[i].transport) == 0);
        CHECK(strcmp(d.protocol, cases[i].protocol) == 0);
        CHECK(strcmp(d.media, cases[i].media) == 0);
        if (strcmp(d.media, "unknown") != 0) CHECK(strcmp(d.media_confidence, "heuristic") == 0);
        CHECK(ds_default_visible(&d) == !(cases[i].facts.partition || cases[i].facts.volume || cases[i].facts.snapshot));
        ds_device_free(&d);
        printf("fixture: %s OK\n", cases[i].name);
    }
}

/** @brief Verify namespace identity and Windows deduplication without relying on serial uniqueness. */
static void identity(void)
{
    ds_inventory inventory = {0};
    for (unsigned int i = 0; i < 3; ++i) {
        ds_device d;
        ds_device_init(&d);
        d.platform = "linux";
        CHECK(ds_text_offer(&d, &d.device_id, i == 1U ? "namespace:1" : "namespace:0", "fixture") == 0);
        d.major_number = 259;
        d.has_devnum = true;
        d.minor_number = i == 2U ? 0U : i;
        CHECK(ds_text_offer(&d, &d.serial, "DUPLICATE", "fixture") == 0);
        CHECK(ds_text_offer(&d, &d.controller, "/sys/example/nvme0", "fixture") == 0);
        CHECK(ds_text_offer(&d, &d.path, i != 1U ? "/dev/nvme0n2" : "/dev/nvme0n1", "fixture") == 0);
        ds_facts f = {.disk=true, .nvme=true};
        CHECK(ds_classify(&d, &f) == 0);
        CHECK(ds_inventory_add(&inventory, &d) == 0);
        ds_device_free(&d);
    }
    CHECK(inventory.count == 2U); /* Two namespaces; alias of dev_t 259:0 deduped. */
    ds_inventory_sort(&inventory);
    CHECK(strcmp(inventory.items[0].path.value, "/dev/nvme0n1") == 0);
    CHECK(strcmp(inventory.items[0].controller.value, inventory.items[1].controller.value) == 0);
    CHECK(strcmp(inventory.items[0].serial.value, inventory.items[1].serial.value) == 0);
    ds_inventory_free(&inventory);
    ds_inventory_sort(&inventory);
    CHECK(inventory.count == 0);
    ds_inventory_free(&inventory);
    /* Windows has no Unix dev_t: unknown zero values must never merge records. */
    for (unsigned int i = 0; i < 3; ++i) {
        ds_device d;
        ds_device_init(&d);
        d.platform = "windows";
        CHECK(ds_text_offer(&d, &d.device_id, i == 1U ? "native:disk1" : "native:disk0", "fixture") == 0);
        CHECK(ds_inventory_add(&inventory, &d) == 0);
        ds_device_free(&d);
    }
    CHECK(inventory.count == 2U);
    ds_inventory_free(&inventory);
}

/** @brief Run the portable core assertions and print the number of successful checks. */
int main(void)
{
    numbers(); normalization(); classifications(); identity();
    printf("%u core assertions passed; 24 classification fixtures.\n", checks);
    return 0;
}
