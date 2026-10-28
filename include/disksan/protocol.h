/** @file
 * @brief Decode fixed protocol metadata without native commands or storage-data access.
 */
#ifndef DISKSAN_PROTOCOL_H
#define DISKSAN_PROTOCOL_H
#include "disksan/assessment.h"

/** @brief The only accepted Identify Controller payload size, in bytes. */
#define DS_NVME_IDENTIFY_SIZE 4096U
/** @brief Closed request identifiers; zero and every unlisted value are invalid. */
typedef enum { DS_QUERY_NVME_IDENTIFY_CONTROLLER = 1 } ds_query_kind;
/** @brief Decode failures cannot establish absent hardware support. */
typedef enum {
    DS_NVME_DECODE_OK, DS_NVME_DECODE_LENGTH,
    DS_NVME_DECODE_VERSION, DS_NVME_DECODE_FIELDS
} ds_nvme_decode_status;
/** @brief Retain only bounded numeric facts; no serial, model or raw response escapes. */
typedef struct {
    uint32_t version; /**< @brief Recognized NVMe version register value. */
    uint32_t namespace_limit; /**< @brief Reported maximum valid namespace identifier. */
    uint16_t controller_id; /**< @brief Controller identifier for native association checks, not global identity. */
    uint8_t sanitize_support; /**< @brief SANICAP bits 2:0 only; no execution/access claim. */
} ds_nvme_controller;

/**
 * @brief Accept only the individually audited fixed Identify Controller request kind.
 * @param[in] query Request identifier; arbitrary casts and unknown values are rejected.
 * @return True only for DS_QUERY_NVME_IDENTIFY_CONTROLLER; performs no OS operation.
 */
bool ds_protocol_query_allowed(ds_query_kind query);
/**
 * @brief Decode a complete Identify Controller reply with version and field guards.
 * @param[in] bytes Borrowed possibly unaligned bytes; null is rejected without reading.
 * @param[in] size Available byte count; must equal DS_NVME_IDENTIFY_SIZE exactly.
 * @param[out] controller Non-null numeric output, cleared before validation and on failure.
 * @return Decode status; only OK permits interpreting sanitize_support, including zero.
 * @note Native adapters must separately establish the protocol, request and target association.
 */
ds_nvme_decode_status ds_nvme_identify_decode(const unsigned char *bytes, size_t size, ds_nvme_controller *controller);
/**
 * @brief Retain 24 independent capability dimensions from a validated, associated reply.
 * @param[in] controller Non-null successfully decoded controller whose association was revalidated by the caller.
 * @param[in,out] result Initialized assessment with no existing capability entries; owns all copied evidence.
 * @return 0 on success; -1 on invalid input, mixed providers or exhausted evidence, poisoning result.
 * @note Only the three NVMe support states are established; all other dimensions remain unknown.
 */
int ds_nvme_capabilities(const ds_nvme_controller *controller, ds_assessment *result);
/**
 * @brief Validate receipt enum bounds and consistency before rendering or accepting native evidence.
 * @param[in] report Non-null borrowed receipt; zero initialization represents not requested.
 * @return True only for a consistent receipt; this does not validate the native hardware itself.
 */
bool ds_probe_report_valid(const ds_probe_report *report);
/**
 * @brief Return the stable query outcome label.
 * @param[in] status Query status, or any invalid enum value.
 * @return Static label; invalid for unrecognized values.
 */
const char *ds_probe_status_name(ds_probe_status status);
/**
 * @brief Return the fixed native provider label without exposing a path or controller identifier.
 * @param[in] provider Provider enum, or any invalid enum value.
 * @return Static label; invalid for unrecognized values.
 */
const char *ds_probe_provider_name(ds_probe_provider provider);
/**
 * @brief Return a stable machine-readable query reason.
 * @param[in] reason Reason enum, or any invalid enum value.
 * @return Static label; invalid for unrecognized values.
 */
const char *ds_probe_reason_name(ds_probe_reason reason);
/**
 * @brief Explain a query outcome without revealing untrusted native response text.
 * @param[in] reason Reason enum, or any invalid enum value.
 * @return Static bounded explanation; invalid for unrecognized values.
 */
const char *ds_probe_reason_detail(ds_probe_reason reason);
/**
 * @brief Collect opt-in protocol evidence inside the existing target observation window.
 * @param[in] device Borrowed selected device; no writable handle is transferred.
 * @param[in,out] result Initialized assessment with no existing capability provider entries.
 * @param[out] error Non-null bounded diagnostic buffer without private native identifiers.
 * @param[in] size Positive byte capacity of error.
 * @return 0 for completed inspection including unavailable query; 1 system or 3 changed target.
 */
int ds_collect_probed_capabilities(const ds_device *device, ds_assessment *result, char *error, size_t size);
/**
 * @brief Run only a closed audited native query after verifying the selected target's association.
 * @param[in] device Borrowed selected snapshot; no raw command parameters are accepted.
 * @param[in] query Closed request kind; reject unrecognized values before opening any native handle.
 * @param[out] controller Non-null numeric facts, cleared unless a valid associated reply completes.
 * @param[out] report Non-null query receipt with static outcome enums and no raw data.
 * @param[out] error Non-null bounded fatal diagnostic buffer.
 * @param[in] size Positive capacity of error in bytes.
 * @return 0 for completed/unavailable inspection; 1 system/contract error or 3 changed/mismatched target.
 * @note Platform implementation owns and closes every temporary handle; no future identity is guaranteed.
 */
int ds_native_protocol_probe(const ds_device *device, ds_query_kind query, ds_nvme_controller *controller,
                             ds_probe_report *report, char *error, size_t size);
#endif
