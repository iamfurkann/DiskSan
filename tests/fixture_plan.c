/** @file
 * @brief Supply explicitly synthetic planning snapshots and native failures without any operating-system device calls.
 */
#include "disksan/discovery.h"
#include "disksan/target.h"
#include <stdlib.h>
#include <string.h>

static unsigned resolutions;
static const char *selected;
/** @brief Abort an unexpected native sequence before a fixture could conceal a dispatch failure. */
#define REQUIRE(x) do { if (!(x)) { fprintf(stderr, "Planning fixture failed at %d.\n", __LINE__); exit(98); } } while (0)
/** @brief Resolve fake snapshots, preserving an opaque selector as metadata rather than executing it. */
int ds_discover(ds_inventory *inventory, const char *selector, char *error, size_t size)
{
    selected = selector ? selector : "complete"; REQUIRE(strcmp(selected, "never-discover") != 0);
    ++resolutions; REQUIRE(resolutions <= 2U);
    int rc = !strcmp(selected, "missing") ? 3 : !strcmp(selected, "denied") ? 4 : !strcmp(selected, "system-failure") ? 1 : 0;
    if (!strcmp(selected, "after-missing") && resolutions == 2U) rc = 3;
    if (rc) { (void)snprintf(error, size, "Synthetic native failure."); return rc; }
    if (!strcmp(selected, "after-empty") && resolutions == 2U) return 0;
    ds_device d; ds_device_init(&d);
    d.platform = "linux"; d.role = !strcmp(selected, "logical") ? "volume" : "disk";
    d.protocol = "nvme"; d.transport = "nvme"; d.has_devnum = true; d.major_number = 259; d.minor_number = 7;
    const char *identity = !strcmp(selected, "changed") && resolutions == 2U ? "synthetic#diskseq=8" : "synthetic#diskseq=7";
    if (!strcmp(selected, "insufficient")) identity = NULL;
    const char *path = !strcmp(selected, "path-reused") && resolutions == 2U ? "/synthetic/replaced" : selected;
    rc = ds_text_offer(&d, &d.path, path, "fixture") ||
         ds_text_offer(&d, &d.device_id, identity, "sysfs:syspath+diskseq") ||
         ds_text_offer(&d, &d.model, "Synthetic UTF-8:\xc3\xa9 invalid:\xff", "fixture") ||
         ds_number_offer(&d, &d.size_bytes, UINT64_MAX, "fixture") ||
         ds_number_offer(&d, &d.read_only, !strcmp(selected, "read-only") ? 1U : 0U, "fixture");
    if (!rc && ds_inventory_add(inventory, &d)) rc = 1;
    ds_device_free(&d);
    return rc;
}
/** @brief Add independent synthetic completeness or observed hazards within the original target window. */
int ds_collect_usage(const ds_device *device, ds_assessment *result, char *error, size_t size)
{
    (void)device; REQUIRE(resolutions == 1U);
    if (!strcmp(selected, "usage-failure")) { (void)snprintf(error, size, "Synthetic usage failure."); return 1; }
    for (unsigned c = DS_CHECK_TOPOLOGY; c < DS_CHECK_COUNT; ++c) {
        bool hazard = c >= DS_CHECK_MOUNTED && c <= DS_CHECK_USB_BRIDGE;
        ds_truth state = hazard ? DS_FALSE : DS_TRUE;
        if (!strcmp(selected, "review") && c == DS_CHECK_HOST_SCOPE) state = DS_UNKNOWN;
        if ((!strcmp(selected, "mounted") || !strcmp(selected, "conflict")) && c == DS_CHECK_MOUNTED) state = DS_TRUE;
        if (!strcmp(selected, "system") && c == DS_CHECK_SYSTEM) state = DS_TRUE;
        if (!strcmp(selected, "swap") && c == DS_CHECK_SWAP) state = DS_TRUE;
        if (ds_assessment_observe(result, (ds_check)c, state, "synthetic:usage", "Fixture observation only.\n\033[31m")) return 1;
    }
    if (!strcmp(selected, "conflict") && ds_assessment_observe(result, DS_CHECK_MOUNTED, DS_FALSE,
                                                               "synthetic:conflict", "Conflicting fixture.")) return 1;
    return 0;
}
/** @brief Populate all dimensions without protocol access; selected alternatives preserve uncertainty and absence. */
int ds_collect_capabilities(const ds_device *device, ds_assessment *result)
{
    (void)device; REQUIRE(resolutions == 1U);
    if (!strcmp(selected, "cap-failure")) return -1;
    for (unsigned m = 0; m < DS_METHOD_COUNT; ++m) for (unsigned d = 0; d < DS_CAP_DIMENSION_COUNT; ++d) {
        ds_cap_state state = !strcmp(selected, "unknown-capabilities") ? DS_CAP_UNKNOWN : DS_CAP_SUPPORTED;
        if (m == 0U && d == DS_CAP_SUPPORT && !strcmp(selected, "unsupported")) state = DS_CAP_UNSUPPORTED;
        if (m == 0U && d == DS_CAP_ACCESS && !strcmp(selected, "unavailable")) state = DS_CAP_UNAVAILABLE;
        if (ds_assessment_capability(result, (ds_method)m, (ds_cap_dimension)d, state,
                                    "synthetic:capabilities", "Fixture dimension; no protocol call or operation.")) return -1;
    }
    return 0;
}
