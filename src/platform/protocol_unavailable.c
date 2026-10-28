/** @file
 * @brief Decline protocol queries on builds without an approved native provider, without opening a device.
 */
#include "disksan/protocol.h"
#include <string.h>

/** @brief Return a truthful unavailable receipt with no native access or substitute protocol command. */
int ds_native_protocol_probe(const ds_device *device, ds_query_kind query, ds_nvme_controller *controller,
                             ds_probe_report *report, char *error, size_t size)
{
    (void)device;
    memset(controller, 0, sizeof(*controller)); memset(report, 0, sizeof(*report));
    if (!ds_protocol_query_allowed(query)) {
        (void)snprintf(error, size, "Unapproved protocol query."); return 1;
    }
    report->status = DS_PROBE_UNAVAILABLE; report->reason = DS_PROBE_PLATFORM;
    return 0;
}
