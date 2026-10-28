/** @file
 * @brief Select one opt-in capability provider and preserve independent unknowns for unavailable queries.
 */
#include "disksan/protocol.h"

/** @brief Reject mixed or inconsistent providers before publishing any partial capability evidence. */
static int probe_error(ds_assessment *result, char *error, size_t size)
{
    result->failed = true; result->status = DS_INSPECTION_NOT_EVALUATED; result->finding_count = 0;
    (void)snprintf(error, size, "Invalid or incomplete protocol evidence.");
    return 1;
}
/** @brief Run one fixed provider within the caller's observation window and normalize only validated facts. */
int ds_collect_probed_capabilities(const ds_device *device, ds_assessment *result, char *error, size_t size)
{
    if (result->failed || result->capability_count) return probe_error(result, error, size);
    ds_nvme_controller controller = {0}; ds_probe_report report = {0};
    int rc = ds_native_protocol_probe(device, DS_QUERY_NVME_IDENTIFY_CONTROLLER, &controller, &report, error, size);
    result->probe = report;
    if (rc) return rc;
    if (!ds_probe_report_valid(&report) || report.status == DS_PROBE_NOT_REQUESTED) return probe_error(result, error, size);
    if (report.status == DS_PROBE_COMPLETE)
        return ds_nvme_capabilities(&controller, result) ? probe_error(result, error, size) : 0;
    static const char *const limitations[] = {
        "No valid, associated protocol reply established method support.",
        "Sanitization method access is not established by an identification query.",
        "Applicability to the selected namespace and physical media remains unknown.",
        "No complete bridge or transport passthrough proof was obtained."
    };
    for (unsigned m = 0; m < DS_METHOD_COUNT; ++m) {
        for (unsigned d = 0; d < DS_CAP_DIMENSION_COUNT; ++d) {
            bool denied = m < DS_METHOD_ATA_SECURITY && d == DS_CAP_ACCESS && report.reason == DS_PROBE_PERMISSION;
            const char *detail = d == DS_CAP_SUPPORT || denied ? ds_probe_reason_detail(report.reason) : limitations[d];
            if (ds_assessment_capability(result, (ds_method)m, (ds_cap_dimension)d,
                    denied ? DS_CAP_UNAVAILABLE : DS_CAP_UNKNOWN, "provider:audited_probe", detail))
                return probe_error(result, error, size);
        }
    }
    return 0;
}
