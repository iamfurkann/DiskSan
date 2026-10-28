/**
 * @file
 * @brief Revalidate target snapshots around read-only observations without authorization tokens.
 */
#include "disksan/target.h"
#include "disksan/discovery.h"
#include "disksan/protocol.h"

/** @brief Release a failed window and prevent evaluation of any retained partial observations. */
static int fail_window(ds_inventory *target, ds_assessment *result, int rc)
{
    ds_inventory_free(target);
    result->failed = true; result->status = DS_INSPECTION_NOT_EVALUATED; result->finding_count = 0;
    return rc;
}
/** @brief Keep fresh native selection, usage and final identity within one explicit observation window. */
static int assess_window(ds_inventory *target, ds_assessment *result, const char *selector, bool probe, char *error, size_t size)
{
    ds_assessment_init(result);
    if (size) error[0] = '\0';
    if (!selector || !*selector) {
        (void)snprintf(error, size, "A nonempty native device selector is required.");
        return fail_window(target, result, 2);
    }
    int rc = ds_discover(target, selector, error, size);
    if (rc) return fail_window(target, result, rc);
    if (target->count != 1U) {
        (void)snprintf(error, size, "Target is missing or ambiguous.");
        return fail_window(target, result, 3);
    }
    ds_device *device = &target->items[0];
    if (ds_assessment_seed(result, device)) {
        (void)snprintf(error, size, "Cannot retain target evidence.");
        return fail_window(target, result, 1);
    }
    rc = ds_collect_usage(device, result, error, size);
    if (rc) return fail_window(target, result, rc);
    if (probe) {
        rc = ds_collect_probed_capabilities(device, result, error, size);
        if (rc) return fail_window(target, result, rc);
    } else if (ds_collect_capabilities(device, result)) {
        (void)snprintf(error, size, "Cannot retain capability evidence.");
        return fail_window(target, result, 1);
    }
    ds_inventory after = {0};
    rc = ds_discover(&after, selector, error, size);
    ds_target_status status = DS_TARGET_PARTIAL;
    if (!rc && after.count != 1U) {
        rc = 3; (void)snprintf(error, size, "Target disappeared or became ambiguous during inspection.");
    }
    if (!rc && ds_target_compare(device, &after.items[0], result, &status)) {
        rc = 1; (void)snprintf(error, size, "Cannot retain identity comparison.");
    }
    if (!rc && status == DS_TARGET_CHANGED) {
        rc = 3; (void)snprintf(error, size, "Target identity or relevant state changed during inspection; retry.");
    }
    if (!rc && (target->warnings.count || after.warnings.count) &&
        ds_assessment_observe(result, DS_CHECK_COLLECTION, DS_UNKNOWN, "resolver:scan_warnings", "Native discovery reported incomplete observations.")) rc = 1;
    ds_inventory_free(&after);
    if (!rc && ds_assessment_evaluate(result)) {
        rc = 1; (void)snprintf(error, size, "Incomplete or invalid assessment evidence.");
    }
    if (rc) return fail_window(target, result, rc);
    ds_inventory_sort(target);
    return 0;
}
/** @brief Preserve metadata-only assessment with no protocol-query invocation. */
int ds_assess_target(ds_inventory *target, ds_assessment *result, const char *selector, char *error, size_t size)
{
    return assess_window(target, result, selector, false, error, size);
}
/** @brief Select the audited protocol provider only for an explicit probing operation. */
int ds_probe_capabilities_target(ds_inventory *target, ds_assessment *result, const char *selector, char *error, size_t size)
{
    return assess_window(target, result, selector, true, error, size);
}
