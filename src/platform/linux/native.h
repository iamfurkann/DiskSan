/** @file
 * @brief Separate Linux record collection from enumeration for deterministic boundary tests.
 */
#ifndef DISKSAN_LINUX_NATIVE_H
#define DISKSAN_LINUX_NATIVE_H
#include "disksan/device.h"
struct udev_device;
/** @brief Collect borrowed udev metadata; return 0, 1 for changed identity, or -1 for allocation failure. */
int ds_linux_collect(struct udev_device *u, ds_device *d);
#endif
