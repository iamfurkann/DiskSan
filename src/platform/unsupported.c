/**
 * @file
 * @brief Report unsupported hosts without fabricating successful disk discovery.
 */
#include "disksan/discovery.h"
#include "disksan/target.h"
#include <stdio.h>

/** @brief Return a system error explaining that this host has no native discovery adapter. */
int ds_discover(ds_inventory *inventory, const char *selector, char *error, size_t size)
{
    (void)inventory;
    (void)selector;
    (void)snprintf(error, size, "Live discovery requires Linux, macOS or Windows; this host supports portable tests only.");
    return 1;
}
/** @brief Fail unsupported usage collection rather than fabricating an empty successful assessment. */
int ds_collect_usage(const ds_device *device, ds_assessment *result, char *error, size_t size)
{
    (void)device; (void)result;
    (void)snprintf(error, size, "Live assessment requires Linux, macOS or Windows.");
    return 1;
}
