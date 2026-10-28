/** @file
 * @brief Substitute discovery, usage and protocol boundaries for end-to-end tests without physical hardware.
 */
#include "disksan/discovery.h"
#include "disksan/protocol.h"
#include "disksan/target.h"
#include "probe_fixture.h"
#include <stdlib.h>
#include <string.h>

static unsigned resolutions, queries;
static const char *selected;
/** @brief Abort incorrect sequencing instead of permitting a quiet native query or extra resolution. */
#define REQUIRE(x) do { if (!(x)) { fprintf(stderr, "Fixture contract failed at %d.\n", __LINE__); exit(98); } } while (0)
/** @brief Reset independent fake windows without retaining borrowed selector pointers. */
void ds_test_probe_reset(void) { resolutions = queries = 0; selected = NULL; }
/** @brief Report the synthetic protocol invocation count. */
unsigned ds_test_probe_calls(void) { return queries; }
/** @brief Report how many times the original selector was resolved. */
unsigned ds_test_probe_resolutions(void) { return resolutions; }
/** @brief Resolve only synthetic records and inject selector, identity and scope changes. */
int ds_discover(ds_inventory *inventory, const char *selector, char *error, size_t size)
{
    selected = selector ? selector : "no-probe";
    REQUIRE(strcmp(selected, "never-resolve") != 0);
    ++resolutions; REQUIRE(resolutions <= 2U);
    int rc = !strcmp(selected, "missing") || !strcmp(selected, "regular-file") || !strcmp(selected, "directory") ? 3 :
             !strcmp(selected, "permission") ? 4 : 0;
    if (!strcmp(selected, "after-missing") && resolutions == 2U) rc = 3;
    if (rc) { (void)snprintf(error, size, "Synthetic selector failure."); return rc; }
    if (!strcmp(selected, "after-empty") && resolutions == 2U) return 0;
    ds_device device; ds_device_init(&device);
    device.platform = "linux"; device.role = "namespace"; device.protocol = "nvme"; device.transport = "pcie";
    device.has_devnum = true; device.major_number = 259; device.minor_number = 1;
    const char *controller = !strcmp(selected, "changed") && resolutions == 2U ? "/sys/fixture/other-controller" : "/sys/fixture/controller";
    REQUIRE(!ds_text_offer(&device, &device.path, "/dev/fixture-nvme", "fixture"));
    REQUIRE(!ds_text_offer(&device, &device.controller, controller, "fixture"));
    REQUIRE(!ds_text_offer(&device, &device.device_id, "/sys/fixture#diskseq=7", "sysfs:syspath+diskseq"));
    REQUIRE(!ds_number_offer(&device, &device.size_bytes, 4096, "fixture"));
    REQUIRE(!ds_inventory_add(inventory, &device)); ds_device_free(&device);
    return 0;
}
/** @brief Add a known hazard so successful protocol evidence cannot silently grant permission. */
int ds_collect_usage(const ds_device *device, ds_assessment *result, char *error, size_t size)
{
    (void)device; REQUIRE(resolutions == 1U && queries == 0U);
    if (!strcmp(selected, "usage-failure")) { (void)snprintf(error, size, "Synthetic usage failure."); return 1; }
    return ds_assessment_observe(result, DS_CHECK_MOUNTED, DS_TRUE, "fixture:mount", "Synthetic active consumer.") ? 1 : 0;
}
/** @brief Inject query outcomes, decode a fixed synthetic reply and never access an OS device. */
int ds_native_protocol_probe(const ds_device *device, ds_query_kind query, ds_nvme_controller *controller,
                             ds_probe_report *report, char *error, size_t size)
{
    REQUIRE(device && ds_protocol_query_allowed(query));
    REQUIRE(resolutions == 1U && queries == 0U && strcmp(selected, "no-probe") != 0);
    ++queries; memset(controller, 0, sizeof(*controller)); memset(report, 0, sizeof(*report));
    if (!strcmp(selected, "probe-changed") || !strcmp(selected, "probe-fatal")) {
        (void)snprintf(error, size, "Synthetic protocol failure.");
        return !strcmp(selected, "probe-changed") ? 3 : 1;
    }
    report->provider = DS_PROVIDER_LINUX_NVME;
    if (!strcmp(selected, "unavailable")) {
        report->status = DS_PROBE_UNAVAILABLE; report->provider = DS_PROVIDER_NONE; report->reason = DS_PROBE_PLATFORM;
    } else if (!strcmp(selected, "denied") || !strcmp(selected, "query-denied")) {
        report->status = DS_PROBE_UNAVAILABLE; report->performed = !strcmp(selected, "query-denied"); report->reason = DS_PROBE_PERMISSION;
    } else if (!strcmp(selected, "query-error") || !strcmp(selected, "version")) {
        report->status = DS_PROBE_UNAVAILABLE; report->performed = true;
        report->reason = !strcmp(selected, "version") ? DS_PROBE_VERSION : DS_PROBE_QUERY_FAILED;
    } else if (!strcmp(selected, "bad-reply")) {
        report->status = DS_PROBE_INVALID_RESPONSE; report->performed = true; report->reason = DS_PROBE_BAD_RESPONSE;
    } else {
        unsigned char bytes[4096] = {0};
        bytes[0] = 1; bytes[81] = 4; bytes[82] = 1; bytes[111] = 1;
        bytes[328] = 3; bytes[331] = 0x40; bytes[516] = 1;
        memcpy(bytes + 4, "PRIVATE-RAW-IDENTIFY", 20);
        REQUIRE(ds_nvme_identify_decode(bytes, sizeof(bytes), controller) == DS_NVME_DECODE_OK);
        report->status = DS_PROBE_COMPLETE; report->performed = true;
        if (!strcmp(selected, "bad-receipt")) report->performed = false;
        if (!strcmp(selected, "bad-decoded")) controller->version = UINT32_MAX;
    }
    return 0;
}
