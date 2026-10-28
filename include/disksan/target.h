/**
 * @file
 * @brief Define an inspection-only target window with explicit identity uncertainty.
 */
#ifndef DISKSAN_TARGET_H
#define DISKSAN_TARGET_H
#include "disksan/assessment.h"
/** @brief Comparison is limited to two snapshots and cannot guarantee future identity. */
typedef enum { DS_TARGET_PARTIAL, DS_TARGET_MATCH, DS_TARGET_CHANGED } ds_target_status;
/**
 * @brief Compare independently resolved records and expose missing identity or generation.
 * @param[in] before Non-null borrowed initialized first observation.
 * @param[in] after Non-null borrowed initialized last observation.
 * @param[in,out] result Initialized assessment receiving copied comparison evidence.
 * @param[out] status Non-null comparison result; changed records must not be published.
 * @return 0 on comparison success; -1 on bounded evidence failure.
 * @note A matching generation is only evidence within this non-atomic window.
 */
int ds_target_compare(const ds_device *before, const ds_device *after, ds_assessment *result, ds_target_status *status);
/**
 * @brief Resolve, observe and re-resolve one selector without returning an access handle.
 * @param[in,out] target Non-null empty initialized inventory; owns the first snapshot only on success.
 * @param[in,out] result Non-null result, initialized by this call and poisoned on failure.
 * @param[in] selector Nonempty borrowed native selector, reused unchanged for both resolutions.
 * @param[out] error Non-null diagnostic buffer; receives no raw inventory or serial metadata.
 * @param[in] size Positive byte capacity of error, including terminator.
 * @return 0 on completed inspection including blocked findings; 1 system, 2 usage,
 *         3 invalid/changed target, or 4 permission. Failure releases partial snapshots.
 */
int ds_assess_target(ds_inventory *target, ds_assessment *result, const char *selector, char *error, size_t size);
/**
 * @brief Explicitly probe capabilities inside the same non-atomic target observation window.
 * @param[in,out] target Non-null empty initialized inventory; owns the selected snapshot only on success.
 * @param[in,out] result Non-null assessment initialized by this call and poisoned on failure.
 * @param[in] selector Nonempty borrowed native selector, reused unchanged for final revalidation.
 * @param[out] error Non-null bounded diagnostic buffer without raw protocol contents.
 * @param[in] size Positive byte capacity of error including terminator.
 * @return 0 for inspection including unavailable probing, or the existing 1/2/3/4 failure classes.
 * @note Only the explicit capabilities --probe CLI path calls this entrypoint; no handle or authority escapes.
 */
int ds_probe_capabilities_target(ds_inventory *target, ds_assessment *result, const char *selector, char *error, size_t size);
/**
 * @brief Collect native read-only usage observations within the target window.
 * @param[in] device Borrowed selected snapshot; no ownership or writable handle is transferred.
 * @param[in,out] result Initialized assessment receiving copied evidence and limitations.
 * @param[out] error Non-null bounded fatal diagnostic buffer.
 * @param[in] size Positive capacity of error in bytes.
 * @return 0 for observations including explicit unknowns; 1 allocation/system,
 *         3 changed target, or 4 fatal selector permission failure.
 * @note Implemented by the selected platform adapter, never by a destructive backend.
 */
int ds_collect_usage(const ds_device *device, ds_assessment *result, char *error, size_t size);
/**
 * @brief Supply honest method-level unknowns using metadata only and issuing no commands.
 * @param[in] device Borrowed selected record; bus labels never establish method support.
 * @param[in,out] result Initialized assessment receiving copied method evidence.
 * @return 0 on success; -1 on evidence capacity/validation failure.
 */
int ds_collect_capabilities(const ds_device *device, ds_assessment *result);
#endif
