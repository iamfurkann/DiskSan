/** @file
 * @brief Declare the single Linux NVMe query primitive; callers retain verified read-only descriptor ownership.
 */
#ifndef DISKSAN_LINUX_PROTOCOL_QUERY_H
#define DISKSAN_LINUX_PROTOCOL_QUERY_H
#include "disksan/protocol.h"
/** @brief Issue one fixed Identify Controller request on an already verified controller descriptor; return raw ioctl status without retries. */
int ds_linux_nvme_identify(int fd, ds_query_kind query, unsigned char response[DS_NVME_IDENTIFY_SIZE]);
#endif
