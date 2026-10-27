/**
 * @file
 * @brief Define bounded read-only evidence and inspection policy without authorization.
 */
#ifndef DISKSAN_ASSESSMENT_H
#define DISKSAN_ASSESSMENT_H
#include "disksan/device.h"

/** @brief Maximum observations per evidence family; overflow poisons the result. */
#define DS_ASSESS_LIMIT 64U
/** @brief Source labels include their terminator within this byte bound. */
#define DS_ASSESS_SOURCE 96U
/** @brief Diagnostic evidence includes its terminator within this byte bound. */
#define DS_ASSESS_DETAIL 256U
/** @brief Unknown is the zero-initialized state and never means absent or safe. */
typedef enum { DS_UNKNOWN, DS_FALSE, DS_TRUE } ds_truth;
/** @brief Independent completeness dimensions and observed usage hazards. */
typedef enum {
    DS_CHECK_IDENTITY, DS_CHECK_GENERATION, DS_CHECK_TOPOLOGY,
    DS_CHECK_USAGE, DS_CHECK_HOST_SCOPE, DS_CHECK_PHYSICAL_SCOPE,
    DS_CHECK_MOUNTED, DS_CHECK_SWAP, DS_CHECK_SYSTEM, DS_CHECK_READ_ONLY,
    DS_CHECK_HOLDERS, DS_CHECK_SLAVES, DS_CHECK_LOGICAL, DS_CHECK_USB_BRIDGE,
    DS_CHECK_COLLECTION, DS_CHECK_COUNT
} ds_check;
/** @brief Informational method names; no method has an executable backend. */
typedef enum {
    DS_METHOD_NVME_BLOCK, DS_METHOD_NVME_CRYPTO, DS_METHOD_NVME_OVERWRITE,
    DS_METHOD_ATA_SECURITY, DS_METHOD_ATA_SANITIZE, DS_METHOD_SCSI_SANITIZE,
    DS_METHOD_COUNT
} ds_method;
/** @brief Support, access, applicability and passthrough must not imply each other. */
typedef enum {
    DS_CAP_SUPPORT, DS_CAP_ACCESS, DS_CAP_APPLICABILITY, DS_CAP_PASSTHROUGH,
    DS_CAP_DIMENSION_COUNT
} ds_cap_dimension;
/** @brief Unavailable describes inspection failure, not unsupported hardware. */
typedef enum {
    DS_CAP_UNKNOWN, DS_CAP_SUPPORTED, DS_CAP_UNSUPPORTED, DS_CAP_UNAVAILABLE
} ds_cap_state;
/** @brief Policy severity is distinct from process exit status. */
typedef enum { DS_SEVERITY_INFO, DS_SEVERITY_REVIEW, DS_SEVERITY_BLOCK } ds_severity;
/** @brief Inspection-only outcomes; none permits a future destructive action. */
typedef enum {
    DS_INSPECTION_NOT_EVALUATED, DS_INSPECTION_NO_BLOCKERS,
    DS_INSPECTION_REVIEW, DS_INSPECTION_BLOCKED
} ds_inspection_status;
/** @brief Own one untrusted observation without borrowing native buffers. */
typedef struct {
    ds_check check; /**< @brief Dimension being observed. */
    ds_truth state; /**< @brief Explicit truth or uncertainty. */
    char source[DS_ASSESS_SOURCE]; /**< @brief Owned provenance label. */
    char detail[DS_ASSESS_DETAIL]; /**< @brief Owned bounded explanation. */
} ds_check_evidence;
/** @brief Own one method/dimension observation, never an execution capability. */
typedef struct {
    ds_method method; /**< @brief Informational method under inspection. */
    ds_cap_dimension dimension; /**< @brief Independent evidence dimension. */
    ds_cap_state state; /**< @brief Observed result or explicit uncertainty. */
    char source[DS_ASSESS_SOURCE]; /**< @brief Owned query/provider provenance. */
    char detail[DS_ASSESS_DETAIL]; /**< @brief Owned scope or limitation. */
} ds_cap_evidence;
/** @brief Derived finding references observations by dimension rather than array position. */
typedef struct {
    const char *code; /**< @brief Static stable machine-readable finding code. */
    const char *category; /**< @brief Static identity/role/usage/topology/capability category. */
    const char *subject; /**< @brief Static check or method key for evidence lookup. */
    const char *message; /**< @brief Static explanatory text without native metadata. */
    ds_severity severity; /**< @brief Blocking or review advice for hypothetical operations. */
    ds_truth state; /**< @brief Derived true/false/unknown with conflicting evidence unknown. */
} ds_finding;
/** @brief Fixed-size owned assessment; safe to copy, clear or release repeatedly. */
typedef struct {
    ds_check_evidence checks[DS_ASSESS_LIMIT]; /**< @brief Append-only check observations. */
    size_t check_count; /**< @brief Initialized check entries. */
    ds_cap_evidence capabilities[DS_ASSESS_LIMIT]; /**< @brief Independent method evidence. */
    size_t capability_count; /**< @brief Initialized capability entries. */
    ds_finding findings[DS_ASSESS_LIMIT]; /**< @brief Derived findings in stable order. */
    size_t finding_count; /**< @brief Initialized findings; reset on every evaluation. */
    ds_inspection_status status; /**< @brief Content status; never authorization. */
    bool failed; /**< @brief Sticky invalid/overflow input prevents evaluation success. */
} ds_assessment;
/**
 * @brief Initialize or clear a fixed-size result; no resources are owned outside it.
 * @param[out] result Non-null result; repeated clearing is safe.
 */
void ds_assessment_init(ds_assessment *result);
/**
 * @brief Retain a bounded check observation or ignore its exact duplicate.
 * @param[in,out] result Initialized result; failed input poisons further evaluation.
 * @param[in] check Valid observation dimension.
 * @param[in] state True, false or unknown; unknown is not a negative result.
 * @param[in] source Nonempty borrowed label, copied without truncation.
 * @param[in] detail Nonempty borrowed explanation, copied without truncation.
 * @return 0 on success; -1 on invalid/oversized input or exhausted storage.
 */
int ds_assessment_observe(ds_assessment *result, ds_check check, ds_truth state, const char *source, const char *detail);
/**
 * @brief Retain independent method evidence without inferring support from access.
 * @param[in,out] result Initialized result; failed input poisons further evaluation.
 * @param[in] method Valid informational method; no command is issued.
 * @param[in] dimension Valid independent evidence dimension.
 * @param[in] state Observed supported/unsupported, unknown or unavailable.
 * @param[in] source Nonempty borrowed provenance, copied without truncation.
 * @param[in] detail Nonempty borrowed explanation, copied without truncation.
 * @return 0 on success or exact duplicate; -1 on invalid input or capacity exhaustion.
 */
int ds_assessment_capability(ds_assessment *result, ds_method method, ds_cap_dimension dimension, ds_cap_state state, const char *source, const char *detail);
/**
 * @brief Compute conservative inspection findings and sort evidence deterministically.
 * @param[in,out] result Initialized observations; failed results cannot be evaluated.
 * @return 0 on evaluated inspection; -1 on invalid/overflow evidence, leaving not_evaluated.
 * @note Neither a positive state nor a saved result authorizes any future operation.
 */
int ds_assessment_evaluate(ds_assessment *result);
/**
 * @brief Seed role and read-only facts from an existing snapshot without assuming completeness.
 * @param[in,out] result Initialized result receiving copied observations.
 * @param[in] device Non-null borrowed initialized device; ownership is unchanged.
 * @return 0 on success; -1 on evidence failure, which poisons evaluation.
 */
int ds_assessment_seed(ds_assessment *result, const ds_device *device);
/**
 * @brief Return the stable key for an observation dimension.
 * @param[in] check Valid dimension or an invalid value.
 * @return Static key, or invalid for an out-of-range value.
 */
const char *ds_check_name(ds_check check);
/**
 * @brief Return a stable informational method name.
 * @param[in] method Valid method or an invalid value.
 * @return Static key, or invalid for an out-of-range value.
 */
const char *ds_method_name(ds_method method);
/**
 * @brief Render a completed inspection; every envelope explicitly denies erase authorization.
 * @param[in,out] out Non-null writable stream; flushed before returning.
 * @param[in] device Borrowed selected schema-2 record.
 * @param[in] result Borrowed successfully evaluated result with sorted observations.
 * @param[in] json Select machine JSON instead of terminal-safe text.
 * @param[in] capabilities Select the capabilities operation label; safety findings remain present.
 * @return 0 on successful rendering; 1 on invalid result, stream or flush failure.
 */
int ds_assessment_output(FILE *out, const ds_device *device, const ds_assessment *result, bool json, bool capabilities);
#endif
