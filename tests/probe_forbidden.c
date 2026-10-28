/** @file
 * @brief Fail existing metadata-only fixtures if they unexpectedly invoke any protocol provider.
 */
#include "disksan/protocol.h"
#include <stdlib.h>

/** @brief Prove default fixture paths cannot quietly start querying protocols. */
int ds_native_protocol_probe(const ds_device *device, ds_query_kind query, ds_nvme_controller *controller,
                             ds_probe_report *report, char *error, size_t size)
{
    (void)device; (void)query; (void)controller; (void)report; (void)error; (void)size;
    fputs("Unexpected native protocol invocation in a metadata-only fixture.\n", stderr);
    exit(99);
}
