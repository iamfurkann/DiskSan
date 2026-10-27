/**
 * @file
 * @brief Inject target-window races and failures with synthetic discovery and usage adapters.
 */
#include "disksan/target.h"
#include "disksan/discovery.h"
#include <stdlib.h>
#include <string.h>

static size_t checks;
static unsigned calls, scenario;
static const char *expected_selector;
/** @brief Fail with a synthetic line number while counting every assertion. */
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)

/** @brief Build a unique synthetic Linux record without any device access. */
static ds_device record(void)
{
    ds_device d; ds_device_init(&d);
    d.platform = "linux"; d.role = "disk";
    d.has_devnum = true; d.major_number = 8; d.minor_number = 0;
    CHECK(ds_text_offer(&d, &d.path, "/dev/synthetic", "fixture") == 0);
    CHECK(ds_text_offer(&d, &d.device_id, "/sys/synthetic#diskseq=42", "sysfs:syspath+diskseq") == 0);
    CHECK(ds_text_offer(&d, &d.parent, "/sys/parent", "fixture") == 0);
    CHECK(ds_text_offer(&d, &d.controller, "/sys/controller", "fixture") == 0);
    CHECK(ds_number_offer(&d, &d.size_bytes, UINT64_MAX, "fixture") == 0);
    CHECK(ds_number_offer(&d, &d.read_only, 0, "fixture") == 0);
    return d;
}
/** @brief Replace one owned fixture field without turning conflict reconciliation into a race. */
static void replace(ds_device *d, ds_text *field, const char *text)
{
    free(field->value); field->value = NULL;
    CHECK(ds_text_offer(d, field, text, "fixture:changed") == 0);
}
/** @brief Simulate fresh selection, changed identities, missing records and partial native failures. */
int ds_discover(ds_inventory *inventory, const char *selector, char *error, size_t size)
{
    CHECK(selector == expected_selector);
    ++calls;
    if (scenario == 10U || scenario == 11U || (scenario == 12U && calls == 2U)) {
        (void)snprintf(error, size, "Synthetic discovery failure.");
        return scenario == 11U ? 4 : scenario == 12U ? 3 : 1;
    }
    if (scenario == 13U && calls == 2U) return 0;
    ds_device d = record();
    if (calls == 2U) {
        if (scenario == 1U) replace(&d, &d.device_id, "/sys/synthetic#diskseq=43");
        if (scenario == 2U) d.minor_number = 1;
        if (scenario == 3U) d.role = "partition";
        if (scenario == 4U) --d.size_bytes.value;
        if (scenario == 5U) replace(&d, &d.parent, "/sys/other-parent");
        if (scenario == 6U) replace(&d, &d.controller, "/sys/other-controller");
        if (scenario == 7U) d.read_only.value = 1;
        if (scenario == 8U) replace(&d, &d.device_id, NULL);
        if (scenario == 9U) CHECK(ds_strings_add(&d.holders, "synthetic-holder") == 0);
    }
    CHECK(ds_inventory_add(inventory, &d) == 0);
    ds_device_free(&d);
    if (scenario == 14U || (scenario == 15U && calls == 2U)) {
        d = record(); d.minor_number = 2;
        replace(&d, &d.path, "/dev/other"); replace(&d, &d.device_id, "/sys/other#diskseq=45");
        CHECK(ds_inventory_add(inventory, &d) == 0);
        ds_device_free(&d);
    }
    if (scenario == 19U) CHECK(ds_strings_add(&inventory->warnings, "synthetic_scan_incomplete") == 0);
    return 0;
}
/** @brief Simulate usage failure, hotplug detection, or evidence exhaustion before final resolution. */
int ds_collect_usage(const ds_device *device, ds_assessment *result, char *error, size_t size)
{
    CHECK(device && device->device_id.value);
    if (scenario == 16U || scenario == 17U) {
        (void)snprintf(error, size, "Synthetic usage failure.");
        return scenario == 16U ? 1 : 3;
    }
    if (scenario == 18U) {
        for (unsigned i = 0; i < DS_ASSESS_LIMIT; ++i) {
            char detail[40]; (void)snprintf(detail, sizeof(detail), "fixture-%u", i);
            if (ds_assessment_observe(result, DS_CHECK_MOUNTED, DS_UNKNOWN, "fixture:exhaust", detail)) return 1;
        }
    }
    return ds_assessment_observe(result, DS_CHECK_MOUNTED, DS_UNKNOWN, "fixture:usage", "No live native metadata queried.") ? 1 : 0;
}
/** @brief Distinguish missing generation, unavailable parents and equal weak labels from proof. */
static void partial_identity(void)
{
    ds_device a = record(), b = record();
    ds_assessment result; ds_assessment_init(&result);
    ds_target_status status;
    CHECK(ds_target_compare(&a, &b, &result, &status) == 0 && status == DS_TARGET_MATCH);
    a.device_id.source = b.device_id.source = "udev:syspath";
    ds_assessment_init(&result);
    CHECK(ds_target_compare(&a, &b, &result, &status) == 0 && status == DS_TARGET_PARTIAL);
    a.platform = b.platform = "windows"; a.has_devnum = b.has_devnum = false;
    a.device_id.source = b.device_id.source = "setupapi:device_interface";
    ds_assessment_init(&result);
    CHECK(ds_target_compare(&a, &b, &result, &status) == 0 && status == DS_TARGET_PARTIAL);
    a.platform = b.platform = "macos";
    a.device_id.source = b.device_id.source = "iokit:registry_entry_id";
    ds_assessment_init(&result);
    CHECK(ds_target_compare(&a, &b, &result, &status) == 0 && status == DS_TARGET_PARTIAL);
    a.has_devnum = b.has_devnum = true;
    ds_assessment_init(&result);
    CHECK(ds_target_compare(&a, &b, &result, &status) == 0 && status == DS_TARGET_MATCH);
    replace(&a, &a.parent, NULL); replace(&b, &b.parent, NULL);
    ds_assessment_init(&result);
    CHECK(ds_target_compare(&a, &b, &result, &status) == 0);
    /* Missing parent is not evidence of complete topology even when native IDs match. */
    CHECK(ds_assessment_evaluate(&result) == 0 && result.status == DS_INSPECTION_REVIEW);
    replace(&a, &a.device_id, NULL); replace(&b, &b.device_id, NULL);
    ds_assessment_init(&result);
    CHECK(ds_target_compare(&a, &b, &result, &status) == 0 && status == DS_TARGET_PARTIAL);
    ds_device_free(&a); ds_device_free(&b);
}
/** @brief Compare unordered links but reject state or geometry loss between observations. */
static void relevant_state(void)
{
    for (unsigned kind = 0; kind < 6; ++kind) {
        ds_device a = record(), b = record();
        if (kind == 0U) b.size_bytes.known = false;
        if (kind == 1U) b.has_devnum = false;
        if (kind == 2U) replace(&b, &b.path, "/dev/replaced");
        if (kind == 3U) replace(&b, &b.parent, NULL);
        if (kind == 4U) b.role = "unknown";
        if (kind == 5U) b.platform = "macos";
        ds_assessment r; ds_assessment_init(&r); ds_target_status status;
        CHECK(ds_target_compare(&a, &b, &r, &status) == 0 && status == DS_TARGET_CHANGED);
        ds_device_free(&a); ds_device_free(&b);
    }
    ds_device a = record(), b = record();
    CHECK(ds_strings_add(&a.holders, "first") == 0 && ds_strings_add(&a.holders, "second") == 0);
    CHECK(ds_strings_add(&b.holders, "second") == 0 && ds_strings_add(&b.holders, "first") == 0);
    ds_assessment r; ds_assessment_init(&r); ds_target_status status;
    CHECK(ds_target_compare(&a, &b, &r, &status) == 0 && status == DS_TARGET_MATCH);
    ds_device_free(&a); ds_device_free(&b);
}
/** @brief Verify every fatal window path frees output records and leaves policy unevaluated. */
static void window_failures(void)
{
    for (scenario = 0; scenario <= 19U; ++scenario) {
        ds_inventory target = {0}; ds_assessment result;
        char error[128]; calls = 0; expected_selector = "/dev/by-id/synthetic-alias";
        int rc = ds_assess_target(&target, &result, expected_selector, error, sizeof(error));
        if (scenario == 0U || scenario == 19U) {
            CHECK(rc == 0 && calls == 2U && target.count == 1U);
            CHECK(result.status == DS_INSPECTION_REVIEW && !result.failed);
            CHECK(result.capability_count == (size_t)DS_METHOD_COUNT * DS_CAP_DIMENSION_COUNT);
        } else {
            int expected = scenario == 10U || scenario == 16U || scenario == 18U ? 1 : scenario == 11U ? 4 : 3;
            CHECK(rc == expected && target.count == 0U && target.items == NULL);
            CHECK(result.failed && result.status == DS_INSPECTION_NOT_EVALUATED);
            CHECK(ds_assessment_evaluate(&result) == -1);
            CHECK(calls == (scenario == 10U || scenario == 11U || scenario == 14U || scenario == 16U || scenario == 17U || scenario == 18U ? 1U : 2U));
        }
        ds_inventory_free(&target); ds_inventory_free(&target);
    }
    ds_inventory target = {0}; ds_assessment result; char error[128]; calls = 0;
    CHECK(ds_assess_target(&target, &result, NULL, error, sizeof(error)) == 2 && calls == 0);
    CHECK(ds_assess_target(&target, &result, "", error, sizeof(error)) == 2 && calls == 0);
}
/** @brief Ensure attractive metadata and writable flags never turn into protocol support. */
static void metadata_capabilities(void)
{
    const char *protocols[] = {"nvme", "ata", "scsi", "unknown"};
    for (size_t i = 0; i < sizeof(protocols) / sizeof(protocols[0]); ++i) {
        ds_device d = record(); d.transport = i == 3U ? "usb" : protocols[i]; d.protocol = protocols[i];
        ds_assessment a; ds_assessment_init(&a);
        CHECK(ds_collect_capabilities(&d, &a) == 0);
        CHECK(a.capability_count == (size_t)DS_METHOD_COUNT * DS_CAP_DIMENSION_COUNT);
        for (size_t j = 0; j < a.capability_count; ++j) CHECK(a.capabilities[j].state == DS_CAP_UNKNOWN);
        CHECK(ds_assessment_evaluate(&a) == 0 && a.status == DS_INSPECTION_REVIEW);
        ds_device_free(&d);
    }
}
/** @brief Run pure identity, bounded metadata provider and injected orchestration tests. */
int main(void)
{
    partial_identity(); relevant_state(); window_failures(); metadata_capabilities();
    printf("Target window: %zu assertions passed.\n", checks);
    return 0;
}
