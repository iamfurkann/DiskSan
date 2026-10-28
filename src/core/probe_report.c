/** @file
 * @brief Validate and label bounded protocol receipts without native operations or private metadata.
 */
#include "disksan/protocol.h"

/** @brief Bound every enum before accepting a consistent query receipt. */
bool ds_probe_report_valid(const ds_probe_report *report)
{
    if ((unsigned)report->status > DS_PROBE_INVALID_RESPONSE ||
        (unsigned)report->provider > DS_PROVIDER_WINDOWS_NVME ||
        (unsigned)report->reason > DS_PROBE_VERSION) return false;
    if (report->performed && report->provider == DS_PROVIDER_NONE) return false;
    switch (report->status) {
        case DS_PROBE_NOT_REQUESTED:
            return !report->performed && report->provider == DS_PROVIDER_NONE && report->reason == DS_PROBE_REASON_NONE;
        case DS_PROBE_COMPLETE:
            return report->performed && report->reason == DS_PROBE_REASON_NONE;
        case DS_PROBE_INVALID_RESPONSE:
            return report->performed && report->reason == DS_PROBE_BAD_RESPONSE;
        case DS_PROBE_UNAVAILABLE:
            if (report->reason == DS_PROBE_PLATFORM)
                return !report->performed && report->provider == DS_PROVIDER_NONE;
            if (report->provider == DS_PROVIDER_NONE) return false;
            if (report->reason == DS_PROBE_TRANSPORT || report->reason == DS_PROBE_IDENTITY)
                return !report->performed;
            if (report->reason == DS_PROBE_VERSION) return report->performed;
            return report->reason == DS_PROBE_PERMISSION || report->reason == DS_PROBE_QUERY_FAILED ||
                   report->reason == DS_PROBE_NATIVE_API;
        default: return false;
    }
}
/** @brief Translate only known status enums to stable output labels. */
const char *ds_probe_status_name(ds_probe_status status)
{
    static const char *const names[] = {"not_requested", "unavailable", "complete", "invalid_response"};
    return (unsigned)status < sizeof(names) / sizeof(names[0]) ? names[status] : "invalid";
}
/** @brief Identify the query mechanism independently of whether it could be attempted. */
const char *ds_probe_provider_name(ds_probe_provider provider)
{
    static const char *const names[] = {"none", "linux_nvme_identify_controller", "windows_nvme_property_query"};
    return (unsigned)provider < sizeof(names) / sizeof(names[0]) ? names[provider] : "invalid";
}
/** @brief Keep machine-readable failure reasons independent of OS errno or raw driver text. */
const char *ds_probe_reason_name(ds_probe_reason reason)
{
    static const char *const names[] = {
        "none", "platform_unavailable", "transport_not_audited", "identity_unverified",
        "permission_denied", "query_failed", "native_api_unavailable", "invalid_response", "version_not_audited"
    };
    return (unsigned)reason < sizeof(names) / sizeof(names[0]) ? names[reason] : "invalid";
}
/** @brief Explain limitations without turning query success into method authorization. */
const char *ds_probe_reason_detail(ds_probe_reason reason)
{
    static const char *const details[] = {
        "The fixed read-only controller query completed; no sanitization method was attempted or authorized.",
        "No audited native protocol provider is enabled for this platform/build.",
        "The selected protocol, transport or storage layer has no approved direct query mapping.",
        "The namespace/controller association or its generation could not be independently established.",
        "The read-only query was denied; no writable handle, privilege escalation or alternate node was attempted.",
        "The native query failed or returned a non-success status; method support remains unknown.",
        "The required native query interface is unavailable; no alternate command mechanism was attempted.",
        "The response failed bounded field or envelope validation; no method support was inferred.",
        "The response version has not been audited by this decoder; method support remains unknown."
    };
    return (unsigned)reason < sizeof(details) / sizeof(details[0]) ? details[reason] : "invalid";
}
