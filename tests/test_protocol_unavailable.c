/** @file
 * @brief Test the actual unsupported-platform adapter and independent unknown evidence without any native device APIs.
 */
#include "disksan/protocol.h"
#include <stdlib.h>
#include <string.h>

static unsigned checks;
/** @brief Stop on a false unavailable-provider contract without logging private device data. */
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)
/** @brief Require unsupported builds to decline even a synthetic NVMe-looking device without inferring support. */
static void unavailable(const char *platform)
{
    ds_device device; ds_device_init(&device);
    device.platform = platform; device.role = "namespace"; device.protocol = "nvme"; device.transport = "nvme";
    ds_nvme_controller controller; memset(&controller, 0xa5, sizeof(controller));
    ds_probe_report report; memset(&report, 0xa5, sizeof(report)); char error[128] = {0};
    CHECK(ds_native_protocol_probe(&device, DS_QUERY_NVME_IDENTIFY_CONTROLLER, &controller, &report, error, sizeof(error)) == 0);
    CHECK(report.status == DS_PROBE_UNAVAILABLE && !report.performed && report.provider == DS_PROVIDER_NONE);
    CHECK(report.reason == DS_PROBE_PLATFORM && ds_probe_report_valid(&report) && error[0] == '\0');
    CHECK(!controller.version && !controller.namespace_limit && !controller.controller_id && !controller.sanitize_support);
    ds_assessment result; ds_assessment_init(&result);
    CHECK(ds_collect_probed_capabilities(&device, &result, error, sizeof(error)) == 0 && !result.failed);
    CHECK(result.capability_count == 24U && result.probe.status == DS_PROBE_UNAVAILABLE && !result.probe.performed);
    for (size_t i = 0; i < result.capability_count; ++i) CHECK(result.capabilities[i].state == DS_CAP_UNKNOWN);
    ds_device_free(&device);
}
/** @brief Reject every unaudited kind even when no native implementation exists. */
static void rejected(void)
{
    ds_device device; ds_device_init(&device);
    for (int kind = -1; kind < 256; ++kind) if (kind != 1) {
        ds_nvme_controller controller; memset(&controller, 0xa5, sizeof(controller));
        ds_probe_report report; char error[128] = {0};
        CHECK(ds_native_protocol_probe(&device, (ds_query_kind)kind, &controller, &report, error, sizeof(error)) == 1);
        CHECK(error[0] && !controller.version && !controller.namespace_limit && !controller.controller_id && !controller.sanitize_support);
        CHECK(report.status == DS_PROBE_NOT_REQUESTED && !report.performed && report.provider == DS_PROVIDER_NONE);
    }
    ds_device_free(&device);
}
/** @brief Run unavailable-provider behavior on every build, including Linux, using only synthetic records. */
int main(void)
{
    unavailable("macos"); unavailable("windows"); unavailable("linux"); unavailable("unknown"); rejected();
    printf("Unavailable protocol adapter: %u assertions passed.\n", checks); return 0;
}
