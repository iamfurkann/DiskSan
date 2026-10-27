/**
 * @file
 * @brief Substitute native boundaries while exercising the production CLI and target window.
 */
#include "disksan/discovery.h"
#include "disksan/target.h"
#include <string.h>

static unsigned calls;
static const char *selected;
/** @brief Resolve a synthetic target or inject a documented native failure class. */
int ds_discover(ds_inventory *inventory, const char *selector, char *error, size_t size)
{
    selected = selector ? selector : "stable"; ++calls;
    int rc = !strcmp(selected, "denied") ? 4 : !strcmp(selected, "missing") ? 3 : !strcmp(selected, "system-failure") ? 1 : 0;
    if (rc) { (void)snprintf(error, size, "Synthetic native failure."); return rc; }
    ds_device d; ds_device_init(&d); d.platform = "linux"; d.role = "disk";
    d.has_devnum = true; d.major_number = 8; d.minor_number = 0;
    const char *identity = !strcmp(selected, "changed") && calls > 1U ? "synthetic:new-generation" : "synthetic:identity";
    if (!strcmp(selected, "partial")) identity = NULL;
    if (ds_text_offer(&d, &d.path, "/synthetic/path\n\033[31m", "fixture") ||
        ds_text_offer(&d, &d.device_id, identity, "sysfs:syspath+diskseq") ||
        ds_text_offer(&d, &d.model, "UTF-8:\xc3\xa9 invalid:\xff", "fixture") ||
        ds_number_offer(&d, &d.size_bytes, UINT64_MAX, "fixture")) rc = 1;
    if (!rc && ds_inventory_add(inventory, &d)) rc = 1;
    ds_device_free(&d);
    return rc;
}
/** @brief Inject usage observations without calling any OS device or filesystem API. */
int ds_collect_usage(const ds_device *device, ds_assessment *result, char *error, size_t size)
{
    (void)device; (void)error; (void)size;
    if (!strcmp(selected, "usage-failure")) return 1;
    return ds_assessment_observe(result, DS_CHECK_MOUNTED, DS_TRUE, "fixture:mounts", "Synthetic active consumer.\n\033[31m") ? 1 : 0;
}
