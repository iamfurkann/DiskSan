/** @file
 * @brief Verify opt-in target-window ordering, discarded failures and bounded receipt rendering with synthetic adapters.
 */
#include "disksan/target.h"
#include "disksan/protocol.h"
#include "probe_fixture.h"
#include <stdlib.h>
#include <string.h>

static unsigned checks;
/** @brief Fail a bounded synthetic assertion without reporting any native inventory. */
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)
/** @brief Require fatal windows to retain neither a target nor a publishable partial payload. */
static void failures(void)
{
    const char *const cases[] = {"missing", "regular-file", "directory", "permission", "usage-failure",
        "probe-changed", "probe-fatal", "bad-receipt", "bad-decoded", "changed", "after-missing", "after-empty"};
    const int statuses[] = {3, 3, 3, 4, 1, 3, 1, 1, 1, 3, 3, 3};
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        ds_test_probe_reset(); ds_inventory inventory = {0}; ds_assessment result; char error[256] = {0};
        CHECK(ds_probe_capabilities_target(&inventory, &result, cases[i], error, sizeof(error)) == statuses[i]);
        CHECK(inventory.count == 0 && result.failed && result.finding_count == 0 && error[0]);
        CHECK(ds_test_probe_calls() == (i < 5U ? 0U : 1U));
        CHECK(ds_test_probe_resolutions() == (i < 9U ? 1U : 2U));
        FILE *out = tmpfile(); CHECK(out != NULL);
        ds_device empty; ds_device_init(&empty);
        CHECK(ds_assessment_output(out, &empty, &result, true, true) == 1 && ftell(out) == 0L);
        CHECK(fclose(out) == 0); ds_device_free(&empty); ds_inventory_free(&inventory);
    }
    ds_test_probe_reset(); ds_inventory inventory = {0}; ds_assessment result; char error[256];
    CHECK(ds_probe_capabilities_target(&inventory, &result, "", error, sizeof(error)) == 2);
    CHECK(ds_test_probe_calls() == 0U && ds_test_probe_resolutions() == 0U);
}
/** @brief Verify both successful and unavailable queries remain inside the two-resolution window. */
static void completed(void)
{
    const char *const cases[] = {"complete", "unavailable", "denied", "query-denied", "query-error", "version", "bad-reply"};
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        ds_test_probe_reset(); ds_inventory inventory = {0}; ds_assessment result; char error[256] = {0};
        CHECK(ds_probe_capabilities_target(&inventory, &result, cases[i], error, sizeof(error)) == 0);
        CHECK(ds_test_probe_calls() == 1U && ds_test_probe_resolutions() == 2U);
        CHECK(result.capability_count == 24U && result.status == DS_INSPECTION_BLOCKED);
        CHECK(ds_probe_report_valid(&result.probe));
        FILE *out = tmpfile(); CHECK(out != NULL);
        CHECK(ds_assessment_output(out, &inventory.items[0], &result, true, false) == 1 && ftell(out) == 0L);
        CHECK(ds_assessment_output(out, &inventory.items[0], &result, true, true) == 0);
        CHECK(fclose(out) == 0); ds_inventory_free(&inventory);
    }
    ds_test_probe_reset(); ds_inventory inventory = {0}; ds_assessment result; char error[256] = {0};
    CHECK(ds_assess_target(&inventory, &result, "no-probe", error, sizeof(error)) == 0);
    CHECK(ds_test_probe_calls() == 0U && ds_test_probe_resolutions() == 2U && result.probe.status == DS_PROBE_NOT_REQUESTED);
    ds_inventory_free(&inventory);
}
/** @brief Reject malformed receipts before evaluation or output can imply a completed query. */
static void receipts(void)
{
    ds_probe_report report = {0}; CHECK(ds_probe_report_valid(&report));
    report.status = DS_PROBE_COMPLETE; CHECK(!ds_probe_report_valid(&report));
    report.performed = true; CHECK(!ds_probe_report_valid(&report));
    report.provider = DS_PROVIDER_LINUX_NVME; CHECK(ds_probe_report_valid(&report));
    report.reason = DS_PROBE_PERMISSION; CHECK(!ds_probe_report_valid(&report));
    report.status = DS_PROBE_UNAVAILABLE; CHECK(ds_probe_report_valid(&report));
    report.status = DS_PROBE_INVALID_RESPONSE; CHECK(!ds_probe_report_valid(&report));
    report.reason = DS_PROBE_BAD_RESPONSE; CHECK(ds_probe_report_valid(&report));
    report.provider = (ds_probe_provider)99; CHECK(!ds_probe_report_valid(&report));
    CHECK(!strcmp(ds_probe_status_name((ds_probe_status)-1), "invalid"));
    CHECK(!strcmp(ds_probe_provider_name((ds_probe_provider)-1), "invalid"));
    CHECK(!strcmp(ds_probe_reason_name((ds_probe_reason)-1), "invalid"));
    CHECK(!strcmp(ds_probe_reason_detail((ds_probe_reason)-1), "invalid"));
    ds_assessment result; ds_assessment_init(&result); result.probe = report;
    CHECK(ds_assessment_evaluate(&result) == -1 && result.failed);
    report = (ds_probe_report){DS_PROBE_UNAVAILABLE, false, DS_PROVIDER_NONE, DS_PROBE_PLATFORM};
    CHECK(ds_probe_report_valid(&report));
    report.provider = DS_PROVIDER_LINUX_NVME; CHECK(!ds_probe_report_valid(&report));
    report.reason = DS_PROBE_IDENTITY; CHECK(ds_probe_report_valid(&report));
    report.performed = true; CHECK(!ds_probe_report_valid(&report));
    report.reason = DS_PROBE_TRANSPORT; CHECK(!ds_probe_report_valid(&report));
    report.performed = false; CHECK(ds_probe_report_valid(&report));
    report.reason = DS_PROBE_VERSION; CHECK(!ds_probe_report_valid(&report));
    report.performed = true; CHECK(ds_probe_report_valid(&report));
    ds_assessment_init(&result);
    CHECK(ds_assessment_capability(&result, DS_METHOD_NVME_BLOCK, DS_CAP_SUPPORT, DS_CAP_UNKNOWN, "fixture", "Already populated.") == 0);
    ds_device device; ds_device_init(&device); char error[256] = {0}; ds_test_probe_reset();
    CHECK(ds_collect_probed_capabilities(&device, &result, error, sizeof(error)) == 1 && result.failed);
    CHECK(ds_test_probe_calls() == 0U); ds_device_free(&device);
}
/** @brief Run failure, sequencing, default-non-probing and receipt-model regressions. */
int main(void)
{
    failures(); completed(); receipts();
    printf("Opt-in target window: %u assertions passed.\n", checks); return 0;
}
