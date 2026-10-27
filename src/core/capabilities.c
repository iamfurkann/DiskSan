/**
 * @file
 * @brief Report independently unknown method evidence without opening devices or issuing commands.
 */
#include "disksan/target.h"
#include <string.h>

/** @brief Metadata alone cannot establish method flags, accessibility or bridge behavior. */
int ds_collect_capabilities(const ds_device *device, ds_assessment *result)
{
    bool usb = device->transport && !strcmp(device->transport, "usb");
    const char *details[DS_CAP_DIMENSION_COUNT] = {
        "No independently validated identify/log capability provider is enabled; classification is not method support.",
        "Protocol command access was not attempted; metadata-query access does not establish it.",
        "Controller, namespace and method applicability have not been independently established.",
        usb ? "USB bridge passthrough has not been validated; bus type cannot establish it." :
              "The complete controller/transport path has not been independently validated."
    };
    for (unsigned m = 0; m < DS_METHOD_COUNT; ++m)
        for (unsigned d = 0; d < DS_CAP_DIMENSION_COUNT; ++d)
            if (ds_assessment_capability(result, (ds_method)m, (ds_cap_dimension)d, DS_CAP_UNKNOWN,
                                         "provider:metadata_only", details[d])) return -1;
    return 0;
}
