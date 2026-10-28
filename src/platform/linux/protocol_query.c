/** @file
 * @brief Construct only the audited Linux NVMe Identify Controller metadata query with no caller-controlled command fields.
 */
#include "protocol_query.h"
#include <errno.h>
#include <linux/nvme_ioctl.h>
#include <string.h>
#include <sys/ioctl.h>

_Static_assert(sizeof(uintptr_t) <= sizeof(uint64_t), "NVMe UAPI cannot retain the response pointer");
/** @brief Reject unknown request kinds, zero all fields and issue exactly one bounded data-in Identify Controller query. */
int ds_linux_nvme_identify(int fd, ds_query_kind query, unsigned char response[DS_NVME_IDENTIFY_SIZE])
{
    if (fd < 0 || !response || !ds_protocol_query_allowed(query)) { errno = EINVAL; return -1; }
    struct nvme_passthru_cmd command;
    memset(&command, 0, sizeof(command));
    memset(response, 0, DS_NVME_IDENTIFY_SIZE);
    command.opcode = 0x06U;
    command.cdw10 = 0x01U;
    command.addr = (uint64_t)(uintptr_t)response;
    command.data_len = DS_NVME_IDENTIFY_SIZE;
    command.timeout_ms = 5000U;
    /* All other fields, including NSID, metadata, flags and command dwords, remain zero. */
    return ioctl(fd, NVME_IOCTL_ADMIN_CMD, &command);
}
