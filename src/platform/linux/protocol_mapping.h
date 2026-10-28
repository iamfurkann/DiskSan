/** @file
 * @brief Keep Linux namespace/controller association snapshots private to the fixed protocol adapter.
 */
#ifndef DISKSAN_LINUX_PROTOCOL_MAPPING_H
#define DISKSAN_LINUX_PROTOCOL_MAPPING_H
#include "disksan/protocol.h"
#include <sys/stat.h>

/** @brief Distinguish incomplete association from a contradiction of already selected identity. */
typedef enum {
    DS_LINUX_MAP_OK, DS_LINUX_MAP_UNKNOWN, DS_LINUX_MAP_CHANGED, DS_LINUX_MAP_ERROR
} ds_linux_map_status;
/** @brief Own only the bounded association facts needed while one controller descriptor is open. */
typedef struct {
    char controller_node[DS_TEXT_LIMIT + 1U]; /**< @brief Copied udev devnode, never derived from a namespace name. */
    struct stat controller_stat; /**< @brief Character-node device number and filesystem identity. */
    struct stat namespace_stat; /**< @brief Selected block-node device number and filesystem identity. */
    uint64_t diskseq; /**< @brief Linux block-device generation observed again for each mapping. */
    uint32_t nsid; /**< @brief Namespace identifier from the selected namespace sysfs attribute. */
    uint16_t cntlid; /**< @brief Controller identifier from the actual parent sysfs attribute. */
} ds_linux_protocol_mapping;

/**
 * @brief Recreate udev objects and check direct PCI NVMe ancestry, generation and native node types.
 * @param[in] device Borrowed selected namespace snapshot; contains no command parameters.
 * @param[out] mapping Non-null cleared mapping with owned bounded path storage.
 * @return OK only for a complete association; UNKNOWN for absent evidence, CHANGED for contradiction, ERROR for allocation failure.
 * @note Reads only fixed cached metadata attributes; opens no protocol descriptor and issues no ioctl.
 */
ds_linux_map_status ds_linux_protocol_map(const ds_device *device, ds_linux_protocol_mapping *mapping);
/**
 * @brief Compare observable node identity, generation and namespace/controller association.
 * @param[in] before Non-null complete earlier mapping.
 * @param[in] after Non-null complete fresh mapping.
 * @return True only when all retained facts agree; this is not an atomic identity guarantee.
 */
bool ds_linux_protocol_mapping_equal(const ds_linux_protocol_mapping *before, const ds_linux_protocol_mapping *after);
/**
 * @brief Match an opened descriptor to the previously observed character node.
 * @param[in] mapping Non-null validated mapping.
 * @param[in] descriptor_stat Non-null successful fstat result.
 * @return True only for the same character-node type, device number and filesystem inode.
 */
bool ds_linux_protocol_descriptor_matches(const ds_linux_protocol_mapping *mapping, const struct stat *descriptor_stat);
#endif
