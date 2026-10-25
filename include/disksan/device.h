/**
 * @file
 * @brief Define owned metadata, classification evidence, and portable output APIs.
 */
#ifndef DISKSAN_DEVICE_H
#define DISKSAN_DEVICE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/** @brief Maximum metadata length in bytes; longer identifiers are rejected. */
#define DS_TEXT_LIMIT 4096U

/** @brief Own a deduplicated list of heap-allocated strings. */
typedef struct {
    char **items; /**< @brief Owned array of owned, null-terminated strings. */
    size_t count; /**< @brief Number of initialized entries in items. */
} ds_strings;

/** @brief Store optional owned text with a borrowed evidence-source label. */
typedef struct {
    char *value; /**< @brief Owned text, or null when unavailable. */
    const char *source; /**< @brief Borrowed source label with record lifetime. */
} ds_text;

/** @brief Distinguish an unknown number from a known value of zero. */
typedef struct {
    uint64_t value; /**< @brief Numeric value, meaningful only when known is true. */
    bool known; /**< @brief Whether a valid source supplied the value. */
    const char *source; /**< @brief Borrowed source label, or null when unknown. */
} ds_number;

/** @brief Maximum retained supplemental observations per device. */
#define DS_EVIDENCE_LIMIT 64U
/** @brief Own one supplemental observation; numbers use unsigned decimal text. */
typedef struct {
    char *field; /**< @brief Owned field name. */
    char *source; /**< @brief Owned source name; unknown if absent. */
    char *value; /**< @brief Owned normalized value. */
} ds_observation;

/** @brief Own one snapshot record; identity is not authorization to erase a device. */
typedef struct {
    ds_text path; /**< @brief Native device path accepted by the platform selector. */
    ds_text sysfs_path; /**< @brief Linux sysfs path, absent on other platforms. */
    ds_text model; /**< @brief Reported model name. */
    ds_text vendor; /**< @brief Reported vendor name. */
    ds_text serial; /**< @brief Reported serial; uniqueness is never assumed. */
    ds_text wwn; /**< @brief Reported world-wide name. */
    ds_text eui; /**< @brief Reported extended unique identifier. */
    ds_text nguid; /**< @brief Reported NVMe namespace globally unique identifier. */
    ds_text parent; /**< @brief Nearest observed native parent relationship. */
    ds_text controller; /**< @brief Observed controller path, when available. */
    ds_text device_id; /**< @brief Native identity used within the snapshot. */
    unsigned int major_number; /**< @brief Unix major number when has_devnum is true. */
    unsigned int minor_number; /**< @brief Unix minor number when has_devnum is true. */
    bool has_devnum; /**< @brief Whether the native platform supplied a Unix device number. */
    ds_number size_bytes; /**< @brief Exact capacity in bytes, when known. */
    ds_number logical_sector_size; /**< @brief Logical sector size in bytes. */
    ds_number physical_sector_size; /**< @brief Physical sector size in bytes. */
    ds_number rotational; /**< @brief Optional boolean indicating reported rotational media. */
    ds_number removable; /**< @brief Optional native removable-media boolean. */
    ds_number read_only; /**< @brief Optional device flag, distinct from application mode. */
    ds_strings stable_paths; /**< @brief Observed by-id or native interface aliases. */
    ds_strings holders; /**< @brief Observed Linux holders; empty is not a topology guarantee. */
    ds_strings slaves; /**< @brief Observed Linux backing-device relationships. */
    ds_strings warnings; /**< @brief Deduplicated record-level warning codes. */
    ds_observation *evidence; /**< @brief Owned bounded observations supplementing primary sources. */
    size_t evidence_count; /**< @brief Number of initialized supplemental observations. */
    const char *role; /**< @brief Borrowed disk, namespace, partition, or other role label. */
    const char *transport; /**< @brief Borrowed transport label, or unknown. */
    const char *protocol; /**< @brief Borrowed protocol label, or unknown. */
    const char *media; /**< @brief Borrowed heuristic media label, or unknown. */
    const char *media_confidence; /**< @brief Borrowed heuristic or unknown confidence label. */
    const char *classification_source; /**< @brief Borrowed classification evidence label. */
    const char *platform; /**< @brief Borrowed native platform label. */
} ds_device;

/** @brief Own the records and scan-level warnings of a non-atomic snapshot. */
typedef struct {
    ds_device *items; /**< @brief Owned contiguous array of initialized records. */
    size_t count; /**< @brief Number of initialized records in items. */
    ds_strings warnings; /**< @brief Owned scan-level warning codes. */
} ds_inventory;

/**
 * @brief Initialize empty metadata and unknown classifications.
 * @param[out] d Non-null uninitialized or previously released record.
 * @note Call ds_device_free before reinitializing a record that owns memory.
 */
void ds_device_init(ds_device *d);
/**
 * @brief Release all owned record data and reset the record for reuse.
 * @param[in,out] d Non-null initialized record; repeated cleanup is safe.
 */
void ds_device_free(ds_device *d);
/**
 * @brief Release every record and warning, then reset the inventory.
 * @param[in,out] inventory Non-null zero-initialized or populated inventory.
 */
void ds_inventory_free(ds_inventory *inventory);
/**
 * @brief Copy one bounded string unless it is null or already present.
 * @param[in,out] list Non-null initialized string list.
 * @param[in] value Optional null-terminated input; ownership stays with the caller.
 * @return 0 on success or no change; -1 on allocation or size-limit failure.
 */
int ds_strings_add(ds_strings *list, const char *value);
/**
 * @brief Release strings and storage, leaving an empty list.
 * @param[in,out] list Non-null initialized string list.
 */
void ds_strings_free(ds_strings *list);
/**
 * @brief Sort strings lexicographically in place.
 * @param[in,out] list Non-null initialized string list.
 */
void ds_strings_sort(ds_strings *list);
/**
 * @brief Add a deduplicated warning to a device record.
 * @param[in,out] d Non-null initialized record.
 * @param[in] code Null-terminated warning code, copied into the record.
 * @return 0 on success; -1 if the warning cannot be retained.
 */
int ds_warn(ds_device *d, const char *code);
/**
 * @brief Normalize bounded text while preserving the first valid evidence.
 * @param[in,out] d Non-null record receiving length or conflict warnings.
 * @param[in,out] field Initialized destination field; owns accepted text.
 * @param[in] value Optional borrowed input string.
 * @param[in] source Borrowed label that must outlive the record.
 * @return 0 on success, absence, or a retained warning; -1 on allocation failure.
 */
int ds_text_offer(ds_device *d, ds_text *field, const char *value, const char *source);
/**
 * @brief Retain the first known number and warn about conflicting later values.
 * @param[in,out] d Non-null record receiving conflict warnings.
 * @param[in,out] field Initialized numeric destination.
 * @param[in] value Validated unsigned value; zero remains a known value.
 * @param[in] source Borrowed label that must outlive the record.
 * @return 0 on success or a retained conflict warning; -1 on allocation failure.
 */
int ds_number_offer(ds_device *d, ds_number *field, uint64_t value, const char *source);
/**
 * @brief Parse unsigned decimal text with optional trailing ASCII whitespace.
 * @param[in] text Null-terminated text; null, signs, and leading whitespace fail.
 * @param[out] value Receives the parsed value only on success; null fails.
 * @return True for a valid uint64_t; false for invalid input or overflow.
 */
bool ds_parse_u64(const char *text, uint64_t *value);
/**
 * @brief Convert a decimal count of 512-byte sysfs sectors into bytes.
 * @param[in] text Sector count accepted by ds_parse_u64.
 * @param[out] value Receives bytes only on success; null fails.
 * @return True if parsing and multiplication are both representable.
 */
bool ds_sectors_to_bytes(const char *text, uint64_t *value);
/**
 * @brief Reconcile matching native identities using an allocation-safe temporary record.
 * @param[in,out] inventory Non-null initialized destination inventory.
 * @param[in,out] device Initialized record; reset on success, retained on failure.
 * @return 0 when inserted or merged; -1 on allocation failure; -2 on ambiguous identity.
 * @note Failure preserves both inputs. Matching dev_t alone never authorizes merging.
 * @note Known role or device-number conflicts reject the scan instead of losing records.
 * @note Serial numbers are never deduplication keys. Both identities must share a platform.
 */
int ds_inventory_add(ds_inventory *inventory, ds_device *device);
/**
 * @brief Sort records, relationship lists, and warning codes deterministically.
 * @param[in,out] inventory Non-null initialized inventory.
 */
void ds_inventory_sort(ds_inventory *inventory);
/**
 * @brief Decide whether the default list includes a record's role.
 * @param[in] device Non-null initialized record.
 * @return False for partition, volume, or snapshot; true for other roles.
 */
bool ds_default_visible(const ds_device *device);

/** @brief Collect independent platform facts without conflating transport and media. */
typedef struct {
    bool partition; /**< @brief Evidence of a partition rather than a whole device. */
    bool disk; /**< @brief Evidence of a whole-device record. */
    bool composite; /**< @brief Evidence of an aggregate or storage container. */
    bool virtual_device; /**< @brief Evidence of a virtual block device. */
    bool virtio; /**< @brief Evidence of a virtio transport. */
    bool usb; /**< @brief Evidence of USB transport, not an underlying protocol. */
    bool nvme; /**< @brief Native NVMe evidence. */
    bool ata; /**< @brief Native ATA evidence. */
    bool scsi; /**< @brief SCSI access evidence, not a physical cable guarantee. */
    bool mmc; /**< @brief MMC or SD evidence. */
    bool optical; /**< @brief Optical-media evidence that prevents SSD inference. */
    bool volume; /**< @brief Native logical-volume evidence. */
    bool snapshot; /**< @brief Native snapshot evidence. */
    bool media_reported; /**< @brief A native source supplied a medium-type hint. */
} ds_facts;
/**
 * @brief Classify a record conservatively from independent platform facts.
 * @param[in,out] d Initialized record with optional rotational metadata.
 * @param[in] f Non-null platform evidence; no ownership is transferred.
 * @return 0 on success including uncertainty warnings; -1 on warning allocation failure.
 */
int ds_classify(ds_device *d, const ds_facts *f);
/**
 * @brief Write JSON text with control escaping and malformed UTF-8 replacement.
 * @param[in,out] out Non-null writable stream; caller checks stream errors.
 * @param[in] text Optional null-terminated input; null is serialized as JSON null.
 */
void ds_json_string(FILE *out, const char *text);
/**
 * @brief Write ASCII-safe metadata with bounded display width.
 * @param[in,out] out Non-null writable stream; caller checks stream errors.
 * @param[in] text Optional null-terminated input; null displays as unknown.
 * @param[in] limit Display width before the additional three-character ellipsis.
 */
void ds_terminal_string(FILE *out, const char *text, size_t limit);
/**
 * @brief Serialize a snapshot and report stream or flush failures.
 * @param[in,out] out Non-null writable output stream.
 * @param[in] inventory Non-null initialized inventory; ownership is unchanged.
 * @param[in] json Select JSON instead of human-readable text.
 * @param[in] all Include roles hidden by the default filter.
 * @param[in] detail Expand human-readable records; has no effect on JSON.
 * @return 0 if serialization and flushing succeed; 1 on an output error.
 */
int ds_output(FILE *out, const ds_inventory *inventory, bool json, bool all, bool detail);
#endif
