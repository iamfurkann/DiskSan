/**
 * @file
 * @brief Parse bounded Linux proc usage records without syscalls or native headers.
 */
#ifndef DISKSAN_LINUX_USAGE_PARSE_H
#define DISKSAN_LINUX_USAGE_PARSE_H
#include "disksan/device.h"
/** @brief Maximum accepted proc record bytes, excluding a terminator. */
#define DS_PROC_LINE_LIMIT 16384U
/** @brief Own decoded mount paths and numeric identifiers from one mountinfo line. */
typedef struct {
    unsigned int mount_id; /**< @brief Current mount identifier; not persistent identity. */
    unsigned int parent_id; /**< @brief Mount parent, possibly outside visible scope. */
    unsigned int major_number; /**< @brief Filesystem st_dev major, not a path-name guess. */
    unsigned int minor_number; /**< @brief Filesystem st_dev minor. */
    char root[DS_TEXT_LIMIT + 1U]; /**< @brief Decoded filesystem root within this mount. */
    char mount_point[DS_TEXT_LIMIT + 1U]; /**< @brief Decoded namespace-relative mount point. */
} ds_mount_record;
/** @brief Own one active swap entry without opening its file or block contents. */
typedef struct {
    char path[DS_TEXT_LIMIT + 1U]; /**< @brief Decoded path, requiring separate stat-based resolution. */
    bool file; /**< @brief True for file, false for partition; other types are rejected. */
    uint64_t size_kib; /**< @brief Advertised KiB; parsing does not multiply into bytes. */
    uint64_t used_kib; /**< @brief Used KiB, never greater than advertised size. */
    int priority; /**< @brief Signed priority, including negative kernel-assigned values. */
} ds_swap_record;
/** @brief Parse one length-delimited mountinfo record; on error output is all zero. */
int ds_parse_mountinfo(const char *line, size_t length, ds_mount_record *out);
/** @brief Parse one length-delimited swap entry, not its header; on error output is all zero. */
int ds_parse_swap(const char *line, size_t length, ds_swap_record *out);
/** @brief Recognize the proc swaps header after bounded token validation. */
bool ds_swaps_header(const char *line, size_t length);
#endif
