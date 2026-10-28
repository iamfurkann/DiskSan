/** @file
 * @brief Own one read-only controller descriptor for a fixed Identify query between fresh namespace association checks.
 */
#include "protocol_mapping.h"
#include "protocol_query.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

/** @brief Compare classification labels without accepting absent evidence. */
static bool equal(const char *left, const char *right) { return left && right && strcmp(left, right) == 0; }
/** @brief Classify access and absent-interface errors without inferring unsupported hardware. */
static ds_probe_reason error_reason(int code)
{
    if (code == EACCES || code == EPERM) return DS_PROBE_PERMISSION;
    if (code == ENOTTY || code == ENOSYS || code == EOPNOTSUPP) return DS_PROBE_NATIVE_API;
    return DS_PROBE_QUERY_FAILED;
}
/** @brief Report a fatal native contract or changed-identity failure without exposing private paths. */
static int failure(int code, ds_nvme_controller *controller, char *error, size_t size)
{
    memset(controller, 0, sizeof(*controller));
    (void)snprintf(error, size, "%s", code == 3 ?
        "Protocol target association changed or could not be revalidated." : "Protocol collection failed safely.");
    return code;
}
/** @brief Issue only one exact Identify Controller request after binding a direct namespace to a read-only controller node. */
int ds_native_protocol_probe(const ds_device *device, ds_query_kind query, ds_nvme_controller *controller,
                             ds_probe_report *report, char *error, size_t size)
{
    memset(controller, 0, sizeof(*controller));
    *report = (ds_probe_report){DS_PROBE_UNAVAILABLE, false, DS_PROVIDER_LINUX_NVME, DS_PROBE_TRANSPORT};
    if (!ds_protocol_query_allowed(query)) return failure(1, controller, error, size);
    if (!equal(device->platform, "linux") || !equal(device->role, "namespace") ||
        !equal(device->protocol, "nvme") || !equal(device->transport, "nvme") ||
        device->holders.count || device->slaves.count) return 0;
    ds_linux_protocol_mapping before, after;
    ds_linux_map_status mapped = ds_linux_protocol_map(device, &before);
    report->reason = DS_PROBE_IDENTITY;
    if (mapped == DS_LINUX_MAP_UNKNOWN) return 0;
    if (mapped != DS_LINUX_MAP_OK) return failure(mapped == DS_LINUX_MAP_ERROR ? 1 : 3, controller, error, size);
    int fd = open(before.controller_node, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    int saved = errno;
    if (fd < 0) {
        if (saved == ENOMEM || saved == EMFILE || saved == ENFILE) return failure(1, controller, error, size);
        if (saved == ENOENT || saved == ENODEV || saved == ENXIO || saved == ELOOP || saved == ENOTDIR)
            return failure(3, controller, error, size);
        report->reason = error_reason(saved);
        return 0;
    }
    int rc = 0;
    struct stat descriptor_stat;
    if (fstat(fd, &descriptor_stat)) { rc = 1; goto close_handle; }
    if (!ds_linux_protocol_descriptor_matches(&before, &descriptor_stat)) { rc = 3; goto close_handle; }
    mapped = ds_linux_protocol_map(device, &after);
    if (mapped != DS_LINUX_MAP_OK || !ds_linux_protocol_mapping_equal(&before, &after)) {
        rc = mapped == DS_LINUX_MAP_ERROR ? 1 : 3; goto close_handle;
    }
    unsigned char response[DS_NVME_IDENTIFY_SIZE];
    report->performed = true;
    int status = ds_linux_nvme_identify(fd, query, response);
    saved = errno;
    /* Revalidate even failed replies; a lost association cannot become an unavailable success. */
    mapped = ds_linux_protocol_map(device, &after);
    if (mapped != DS_LINUX_MAP_OK || !ds_linux_protocol_mapping_equal(&before, &after)) {
        rc = mapped == DS_LINUX_MAP_ERROR ? 1 : 3; goto close_handle;
    }
    if (status != 0) {
        if (status < 0 && saved == ENOMEM) { rc = 1; goto close_handle; }
        if (status < 0 && (saved == ENODEV || saved == ENXIO || saved == ENOENT)) rc = 3;
        else report->reason = status < 0 ? error_reason(saved) : DS_PROBE_QUERY_FAILED;
        goto close_handle;
    }
    ds_nvme_decode_status decoded = ds_nvme_identify_decode(response, sizeof(response), controller);
    if (decoded == DS_NVME_DECODE_VERSION) report->reason = DS_PROBE_VERSION;
    else if (decoded != DS_NVME_DECODE_OK) {
        report->status = DS_PROBE_INVALID_RESPONSE; report->reason = DS_PROBE_BAD_RESPONSE;
    } else if (controller->controller_id != before.cntlid || before.nsid > controller->namespace_limit) {
        rc = 3;
    } else {
        report->status = DS_PROBE_COMPLETE; report->reason = DS_PROBE_REASON_NONE;
    }
close_handle:
    /* Linux close is never retried after EINTR: retry could close a reused descriptor. */
    if (close(fd) && !rc) rc = 1;
    if (rc) return failure(rc, controller, error, size);
    if (report->status != DS_PROBE_COMPLETE) memset(controller, 0, sizeof(*controller));
    return 0;
}
