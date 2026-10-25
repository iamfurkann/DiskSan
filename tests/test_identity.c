/** @file
 * @brief Verify identity reconciliation and every core allocation failure without native APIs.
 */
#include "disksan/device.h"
#include <stdlib.h>
#include <string.h>

static size_t budget = SIZE_MAX, live, checks;
/** @brief Count successful assertions and stop with a line-number diagnostic on failure. */
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)
/** @brief Fail one allocation at a selected point in the core object under test. */
static bool permitted(void)
{
    if (budget == SIZE_MAX) return true;
    if (!budget) return false;
    --budget;
    return true;
}
/** @brief Replace malloc only in the separately compiled core test object. */
void *ds_test_malloc(size_t size)
{
    if (!permitted()) return NULL;
    void *p = malloc(size);
    if (p) ++live;
    return p;
}
/** @brief Preserve the original block on injected realloc failure and count new blocks. */
void *ds_test_realloc(void *old, size_t size)
{
    if (!permitted()) return NULL;
    bool fresh = old == NULL;
    void *p = realloc(old, size);
    if (p && fresh) ++live;
    return p;
}
/** @brief Track releases from the instrumented core without changing production allocation. */
void ds_test_free(void *p)
{
    if (p) { CHECK(live > 0); --live; }
    free(p);
}
/** @brief Build a sourced synthetic disk with explicitly known Unix identity. */
static ds_device record(const char *id, unsigned int minor_number)
{
    ds_device d;
    ds_device_init(&d);
    d.platform = "linux"; d.role = "disk";
    d.has_devnum = true; d.major_number = 8; d.minor_number = minor_number;
    CHECK(ds_text_offer(&d, &d.device_id, id, "native") == 0);
    CHECK(ds_text_offer(&d, &d.model, "original", "source:first") == 0);
    CHECK(ds_number_offer(&d, &d.size_bytes, 4096, "source:first") == 0);
    return d;
}
/** @brief Find an exact supplemental field/source/value observation. */
static bool observed(const ds_device *d, const char *field, const char *source, const char *value)
{
    for (size_t i = 0; i < d->evidence_count; ++i) {
        const ds_observation *o = &d->evidence[i];
        if (!strcmp(o->field, field) && !strcmp(o->source, source) && !strcmp(o->value, value)) return true;
    }
    return false;
}
/** @brief Verify rejection preserves both owned objects for identity and role collisions. */
static void collisions(void)
{
    for (unsigned int scenario = 0; scenario < 7; ++scenario) {
        ds_inventory inventory = {0};
        ds_device a = record("native:a", 0), b = record(scenario == 0 ? "native:b" : "native:a", scenario == 1 ? 1U : 0U);
        if (scenario == 2) b.role = "partition";
        if (scenario == 3) { free(b.device_id.value); --live; b.device_id.value = NULL; }
        if (scenario == 4) b.role = "volume";
        if (scenario == 5) b.role = "snapshot";
        if (scenario == 6) b.role = "composite";
        CHECK(ds_inventory_add(&inventory, &a) == 0);
        ds_device before = inventory.items[0], input = b;
        CHECK(ds_inventory_add(&inventory, &b) == -2);
        CHECK(inventory.count == 1 && !memcmp(&before, &inventory.items[0], sizeof(before)));
        CHECK(!memcmp(&input, &b, sizeof(b)));
        ds_device_free(&b); ds_inventory_free(&inventory);
        CHECK(live == 0);
    }
}
/** @brief Exhaust every allocation in a rich duplicate merge and verify rollback or complete evidence. */
static void merge_failures(void)
{
    size_t failures = 0;
    for (size_t point = 0; point < 500; ++point) {
        budget = SIZE_MAX;
        ds_inventory inventory = {0};
        ds_device a = record("native:a", 0), b = record("native:a", 0);
        CHECK(ds_text_offer(&a, &a.path, "/dev/a", "native") == 0);
        CHECK(ds_text_offer(&b, &b.path, "/dev/alias", "alias") == 0);
        CHECK(ds_text_offer(&b, &b.vendor, "additional", "source:second") == 0);
        CHECK(ds_text_offer(&b, &b.model, "alternative", "source:second") == 0);
        CHECK(ds_number_offer(&b, &b.size_bytes, 8192, "source:second") == 0);
        CHECK(ds_strings_add(&b.holders, "holder") == 0);
        CHECK(ds_strings_add(&b.slaves, "slave") == 0);
        CHECK(ds_warn(&b, "additional_warning") == 0);
        CHECK(ds_inventory_add(&inventory, &a) == 0);
        ds_device before = inventory.items[0], input = b;
        size_t before_live = live;
        budget = point;
        int rc = ds_inventory_add(&inventory, &b);
        budget = SIZE_MAX;
        if (rc == -1) {
            ++failures;
            CHECK(live == before_live);
            CHECK(!memcmp(&before, &inventory.items[0], sizeof(before)) && !memcmp(&input, &b, sizeof(b)));
            CHECK(!strcmp(b.vendor.value, "additional") && !strcmp(inventory.items[0].model.value, "original"));
        } else {
            CHECK(rc == 0 && inventory.count == 1 && !b.device_id.value && !b.evidence);
            const ds_device *merged = &inventory.items[0];
            CHECK(!strcmp(merged->vendor.value, "additional") && !strcmp(merged->vendor.source, "source:second"));
            CHECK(!strcmp(merged->model.value, "original") && merged->size_bytes.value == 4096);
            CHECK(merged->stable_paths.count == 2 && merged->holders.count == 1 && merged->slaves.count == 1);
            CHECK(observed(merged, "model", "source:second", "alternative"));
            CHECK(observed(merged, "size_bytes", "source:second", "8192"));
            CHECK(observed(merged, "path", "alias", "/dev/alias"));
            ds_inventory_sort(&inventory);
        }
        ds_device_free(&b); ds_inventory_free(&inventory); CHECK(live == 0);
        if (!rc) break;
    }
    CHECK(failures > 30 && failures < 499);
    printf("%zu duplicate-merge allocation failures verified.\n", failures);
}
/** @brief Verify append/string allocation rollback, bounded evidence, and non-identity serials. */
static void boundaries(void)
{
    for (size_t point = 0; point < 2; ++point) {
        ds_strings list = {0}; budget = point;
        CHECK(ds_strings_add(&list, "new") == -1 && !list.items && !list.count && !live);
    }
    budget = SIZE_MAX;
    ds_inventory inventory = {0};
    ds_device d = record("native:a", 0);
    budget = 0; CHECK(ds_inventory_add(&inventory, &d) == -1 && !inventory.items && d.device_id.value);
    budget = SIZE_MAX; ds_device_free(&d);
    for (unsigned int i = 0; i < 3; ++i) {
        d = record(i ? "native:b" : "native:a", i);
        d.has_devnum = false; d.platform = i == 2 ? "macos" : "windows";
        CHECK(ds_text_offer(&d, &d.serial, "shared", "reported") == 0);
        CHECK(ds_text_offer(&d, &d.controller, "same-controller", "reported") == 0);
        CHECK(ds_inventory_add(&inventory, &d) == 0);
    }
    CHECK(inventory.count == 3);
    ds_inventory_free(&inventory);
    d = record("native:a", 0);
    for (size_t i = 0; i < 80; ++i) {
        char value[32]; (void)snprintf(value, sizeof(value), "%zu", i);
        CHECK(ds_text_offer(&d, &d.model, value, "changing") == 0);
    }
    CHECK(d.evidence_count == DS_EVIDENCE_LIMIT);
    bool limited = false;
    for (size_t i = 0; i < d.warnings.count; ++i) if (!strcmp(d.warnings.items[i], "evidence_limit_reached")) limited = true;
    CHECK(limited); ds_device_free(&d); CHECK(!live);
    /* Sorting ties on paths use platform/native identity, never pointer addresses. */
    for (unsigned int i = 0; i < 2; ++i) {
        d = record(i ? "a" : "z", i);
        CHECK(ds_inventory_add(&inventory, &d) == 0);
    }
    ds_inventory_sort(&inventory); CHECK(!strcmp(inventory.items[0].device_id.value, "a"));
    ds_inventory_free(&inventory); CHECK(!live);
}
/** @brief Verify direct metadata allocation failures and merging partially known native identities. */
static void metadata_failures(void)
{
    for (size_t point = 0; point < 20; ++point) {
        budget = SIZE_MAX;
        ds_device d = record("native:a", 0);
        budget = point;
        int rc = ds_text_offer(&d, &d.model, "different", "source:alternative");
        budget = SIZE_MAX;
        CHECK(!strcmp(d.model.value, "original") && !strcmp(d.model.source, "source:first"));
        if (!rc) CHECK(observed(&d, "model", "source:alternative", "different"));
        ds_device_free(&d); CHECK(!live);
        if (!rc) break;
    }
    ds_device d; ds_device_init(&d);
    budget = 0; CHECK(ds_text_offer(&d, &d.model, "new", "source") == -1 && !d.model.value);
    budget = SIZE_MAX;
    ds_inventory inventory = {0};
    ds_device a = record("native:a", 0), b = record("native:a", 0);
    a.has_devnum = false;
    CHECK(ds_inventory_add(&inventory, &a) == 0 && ds_inventory_add(&inventory, &b) == 0);
    CHECK(inventory.count == 1 && inventory.items[0].has_devnum && inventory.items[0].major_number == 8);
    bool warned = false;
    for (size_t i = 0; i < inventory.items[0].warnings.count; ++i)
        if (!strcmp(inventory.items[0].warnings.items[i], "identity_partially_verified")) warned = true;
    CHECK(warned); ds_inventory_free(&inventory); CHECK(!live);
}

/** @brief Retain conflicting relationships, identifiers, sizes, and their exact provenance. */
static void conflicting_metadata(void)
{
    ds_inventory inventory = {0};
    ds_device a = record("native:a", 0), b = record("native:a", 0);
    ds_text *first[] = {&a.vendor, &a.serial, &a.parent, &a.controller, &a.wwn, &a.eui, &a.nguid};
    ds_text *second[] = {&b.vendor, &b.serial, &b.parent, &b.controller, &b.wwn, &b.eui, &b.nguid};
    const char *names[] = {"vendor", "serial", "parent", "controller", "wwn", "eui", "nguid"};
    for (size_t i = 0; i < 7; ++i) {
        CHECK(ds_text_offer(&a, first[i], "first", "source:first") == 0);
        CHECK(ds_text_offer(&b, second[i], "second", "source:second") == 0);
    }
    CHECK(ds_number_offer(&a, &a.logical_sector_size, 512, "source:first") == 0);
    CHECK(ds_number_offer(&b, &b.logical_sector_size, 4096, "source:second") == 0);
    CHECK(ds_number_offer(&a, &a.physical_sector_size, 4096, "source:first") == 0);
    CHECK(ds_number_offer(&b, &b.physical_sector_size, 8192, "source:second") == 0);
    CHECK(ds_inventory_add(&inventory, &a) == 0 && ds_inventory_add(&inventory, &b) == 0);
    const ds_device *d = &inventory.items[0];
    for (size_t i = 0; i < 7; ++i) {
        CHECK(observed(d, names[i], "source:first", "first"));
        CHECK(observed(d, names[i], "source:second", "second"));
    }
    CHECK(d->logical_sector_size.value == 512 && d->physical_sector_size.value == 4096);
    CHECK(observed(d, "logical_sector_size", "source:second", "4096"));
    CHECK(observed(d, "physical_sector_size", "source:second", "8192"));
    ds_inventory_free(&inventory); CHECK(!live);
}

/** @brief Run deterministic reconciliation, rollback, and resource-lifetime checks. */
int main(void)
{
    collisions(); merge_failures(); boundaries(); metadata_failures(); conflicting_metadata();
    printf("%zu identity/allocation assertions passed; zero live core allocations.\n", checks);
    return 0;
}
