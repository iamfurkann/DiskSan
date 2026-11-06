/** @file
 * @brief Describe bounded inspection-only prerequisites without an executable operation or authorization object.
 */
#ifndef DISKSAN_PLAN_H
#define DISKSAN_PLAN_H
#include "disksan/assessment.h"

/** @brief Every completed state remains non-executable; zero rejects unbuilt results. */
typedef enum {
    DS_PLAN_NOT_EVALUATED, DS_PLAN_INSPECTION_ONLY, DS_PLAN_REVIEW,
    DS_PLAN_INSUFFICIENT, DS_PLAN_BLOCKED
} ds_plan_state;
/** @brief Model outcomes describe prerequisites, never a submitted storage operation. */
typedef enum {
    DS_MODEL_REVIEW, DS_MODEL_OBSERVED, DS_MODEL_BLOCK, DS_MODEL_UNIMPLEMENTED
} ds_model_state;
/** @brief Explicit provenance prevents test fixtures from being presented as live observations. */
typedef enum { DS_PLAN_NATIVE_OBSERVATIONS, DS_PLAN_SYNTHETIC_OBSERVATIONS } ds_plan_origin;
/** @brief Fixed prerequisite identities; none selects a sanitization method or command. */
typedef enum {
    DS_PLAN_TARGET, DS_PLAN_IDENTITY, DS_PLAN_USAGE, DS_PLAN_TOPOLOGY,
    DS_PLAN_METHOD_EVIDENCE, DS_PLAN_PRIVILEGE, DS_PLAN_AUTHORIZATION,
    DS_PLAN_EXCLUSIVITY, DS_PLAN_EXECUTION, DS_PLAN_VERIFICATION,
    DS_PLAN_REQUIREMENT_COUNT
} ds_plan_requirement;
/** @brief Own copied bounded evidence and deterministic diagnoses; no borrowed device strings or native handles. */
typedef struct {
    ds_assessment evidence; /**< @brief Owned observations and rebuilt static policy findings; never caller finding pointers. */
    ds_truth observations[DS_CHECK_COUNT]; /**< @brief Consensus summaries; conflicting or incomplete observations remain unknown. */
    unsigned observation_masks[DS_CHECK_COUNT]; /**< @brief Presence of each truth value, retaining hazards despite conflicting summaries. */
    ds_model_state prerequisites[DS_PLAN_REQUIREMENT_COUNT]; /**< @brief One fixed bounded state for each hypothetical prerequisite. */
    ds_plan_state status; /**< @brief Inspection diagnosis only, never permission or an operation outcome. */
    bool target_observed; /**< @brief A bounded nonempty native path was present in the borrowed snapshot. */
    bool failed; /**< @brief Any invalid or exhausted input clears and poisons the entire diagnosis. */
} ds_plan;

/**
 * @brief Clear an owned inline diagnosis without allocating or acquiring resources.
 * @param[out] result Non-null plan; may be cleared repeatedly.
 */
void ds_plan_init(ds_plan *result);
/**
 * @brief Reconstruct an inspection diagnosis from evaluated observations and their selected device snapshot.
 * @param[in] device Borrowed non-null initialized snapshot from the same completed assessment window.
 * @param[in] assessment Borrowed successfully evaluated input; its derived finding pointers are not trusted or retained.
 * @param[out] result Non-null plan, replaced completely; must not contain or alias assessment.
 * @return 0 for a completed non-executable diagnosis including blockers; -1 for invalid/exhausted evidence, clearing and poisoning result.
 * @note No allocation, native query or OS operation occurs. Evidence strings are copied; derived labels have static lifetime.
 */
int ds_plan_build(const ds_device *device, const ds_assessment *assessment, ds_plan *result);
/**
 * @brief Describe an inspection-only result without readiness or authorization language.
 * @param[in] state Result enum; unrecognized values are rejected as invalid labels.
 * @return Static status string, or invalid for an unknown value.
 */
const char *ds_plan_state_name(ds_plan_state state);
/**
 * @brief Describe a hypothetical model state rather than a real operation result.
 * @param[in] state Bounded prerequisite state, or an unrecognized value.
 * @return Static explanatory label, or invalid.
 */
const char *ds_model_state_name(ds_model_state state);
/**
 * @brief Return a stable prerequisite code for an explicitly modeled concept.
 * @param[in] value Fixed prerequisite enum, or an unrecognized value.
 * @return Static code, or invalid.
 */
const char *ds_plan_requirement_code(ds_plan_requirement value);
/**
 * @brief Return a human explanation of a prerequisite's limited meaning.
 * @param[in] value Fixed prerequisite enum, or an unrecognized value.
 * @return Static bounded explanation, or invalid.
 */
const char *ds_plan_requirement_detail(ds_plan_requirement value);
/**
 * @brief Identify which copied evidence or unimplemented future boundary informs a prerequisite.
 * @param[in] value Fixed prerequisite enum, or an unrecognized value.
 * @return Static scope/provenance reference, or invalid.
 */
const char *ds_plan_requirement_scope(ds_plan_requirement value);
/**
 * @brief Render a checked schema-2 inspection plan that always denies execution and authorization.
 * @param[in,out] out Non-null writable stream; flushed before returning.
 * @param[in] device Borrowed initialized snapshot used to build the plan; caller retains ownership.
 * @param[in] plan Borrowed built diagnosis; invalid or inconsistent results fail before writing.
 * @param[in] json Select JSON instead of escaped terminal output.
 * @param[in] origin Fixed observation provenance chosen by the caller, never inferred from device text.
 * @return 0 for completed output including blockers; 1 for invalid data or stream/flush failure.
 * @note This output is not accepted as input by any execution or authorization API.
 */
int ds_plan_output(FILE *out, const ds_device *device, const ds_plan *plan, bool json, ds_plan_origin origin);
/**
 * @brief Render a deterministic ten-step non-executable model transcript from a checked inspection plan.
 * @param[in,out] out Non-null writable stream; flushed before returning.
 * @param[in] device Borrowed initialized snapshot used to build the plan; ownership remains with caller.
 * @param[in] plan Borrowed built diagnosis; inconsistent or failed results are rejected before output.
 * @param[in] json Select schema-2 JSON instead of escaped terminal output.
 * @param[in] origin Fixed provenance, distinguishing native observations from synthetic fixtures.
 * @return 0 for complete transcript output including blockers; 1 for invalid input or stream failure.
 * @note No delay, native call, operation, authorization or completion verification occurs. No transcript import API exists.
 */
int ds_simulation_output(FILE *out, const ds_device *device, const ds_plan *plan, bool json, ds_plan_origin origin);
#endif
