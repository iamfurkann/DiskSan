/**
 * @file
 * @brief Expose native read-only discovery and the Linux metadata probe.
 */
#ifndef DISKSAN_DISCOVERY_H
#define DISKSAN_DISCOVERY_H
#include "disksan/device.h"
/**
 * @brief Discover all native records or resolve one platform-specific selector.
 * @param[in,out] inventory Non-null empty initialized inventory; caller always frees it.
 * @param[in] selector Optional borrowed native path; null requests enumeration.
 * @param[out] error Non-null buffer for a fatal error message.
 * @param[in] size Capacity of error in bytes, including its null terminator.
 * @return 0 on success (possibly with partial metadata), 1 on a system failure,
 *         3 for an invalid or changed target, or 4 for a selector permission error.
 * @note Warnings retain per-field access failures. A snapshot is not atomic or erase authorization.
 */
int ds_discover(ds_inventory *inventory, const char *selector, char *error, size_t size);
#ifdef __linux__
/**
 * @brief Query capacity and sector sizes after validating an opened block identity.
 * @param[in,out] d Initialized Linux record containing a native path and device number.
 * @return 0 on success or retained access/query warnings; 1 on changed/disappeared identity; -1 on allocation failure.
 * @note Opens O_RDONLY and permits only BLKGETSIZE64, BLKSSZGET, and BLKPBSZGET.
 */
int ds_linux_probe(ds_device *d);
#endif
#endif
