/**
 * @file
 * @brief Report unsupported hosts without fabricating successful disk discovery.
 */
#include "disksan/discovery.h"
#include <stdio.h>

/** @brief Return a system error explaining that this host has no native discovery adapter. */
int ds_discover(ds_inventory *inventory, const char *selector, char *error, size_t size)
{
    (void)inventory;
    (void)selector;
    (void)snprintf(error, size, "Live discovery requires Linux, macOS or Windows; this host supports portable tests only.");
    return 1;
}
