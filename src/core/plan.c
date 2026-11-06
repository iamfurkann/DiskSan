/** @file
 * @brief Derive a bounded inspection diagnosis while keeping future authority and execution absent.
 */
#include "disksan/plan.h"
#include "disksan/protocol.h"
#include <string.h>

/** @brief Static prerequisite descriptions contain neither native command parameters nor executable instructions. */
typedef struct { const char *code; const char *scope; const char *detail; } requirement;
static const requirement requirements[DS_PLAN_REQUIREMENT_COUNT] = {
    {"target_observed", "device:path", "A selected device was observed; its path is not an authorization credential."},
    {"identity_confidence", "checks:identity,generation", "Identity applies only to this non-atomic observation and cannot guarantee a later target."},
    {"usage_status", "checks:usage,host_scope,collection,mounted,swap,system,read_only", "Known consumers block the hypothetical operation; incomplete usage visibility requires review."},
    {"topology_coverage", "checks:topology,physical_scope,holders,slaves,logical,usb_bridge", "Namespace, controller and physical-media scope must not be inferred from capacity, role or transport alone."},
    {"method_evidence", "capabilities:independent_dimensions", "Support, OS access, applicability and passthrough are independent; no method is chosen or recommended."},
    {"privilege_unknown", "future:privilege_boundary", "Future operation privileges have not been checked; read-only inspection cannot establish them."},
    {"independent_authorization_unimplemented", "future:independent_authorization", "Typed confirmation, an old plan or an inspection exit code cannot authorize an operation."},
    {"exclusive_access_unimplemented", "future:exclusive_scope", "No atomic lock or global exclusion of concurrent consumers exists in this model."},
    {"execution_unavailable", "product:no_operation_backend", "No storage operation backend is present or callable; this model cannot be executed."},
    {"verification_unavailable", "product:no_completion_verifier", "No media effect is produced, measured, completed or certified by this model."}
};
_Static_assert(sizeof(ds_plan) < 128U * 1024U, "Bound the inline diagnosis stack footprint");

/** @brief Clear all inline state without allocating memory or retaining prior observations. */
void ds_plan_init(ds_plan *result) { memset(result, 0, sizeof(*result)); }
/** @brief Clear partial diagnoses instead of retaining an apparently successful prefix. */
static int failed(ds_plan *result)
{
    ds_plan_init(result); result->failed = true; return -1;
}
/** @brief Check a bounded optional snapshot string before consulting its contents. */
static bool bounded(const char *text)
{
    if (!text) return true;
    for (size_t i = 0; i <= DS_TEXT_LIMIT; ++i) if (!text[i]) return true;
    return false;
}
/** @brief Retain each truth state so known hazards cannot disappear during consensus reduction. */
static void summarize(ds_plan *result)
{
    for (size_t i = 0; i < result->evidence.check_count; ++i) {
        const ds_check_evidence *e = &result->evidence.checks[i];
        result->observation_masks[e->check] |= 1U << (unsigned)e->state;
    }
    for (unsigned c = 0; c < DS_CHECK_COUNT; ++c) {
        unsigned mask = result->observation_masks[c];
        result->observations[c] = mask == (1U << DS_TRUE) ? DS_TRUE : mask == (1U << DS_FALSE) ? DS_FALSE : DS_UNKNOWN;
    }
}
/** @brief Model completeness and hazards separately; a single positive hazard always blocks. */
static ds_model_state check_scope(const ds_plan *result, const ds_check *positive, size_t positives,
                                  const ds_check *hazard, size_t hazards)
{
    bool incomplete = false;
    for (size_t i = 0; i < positives; ++i) incomplete |= result->observations[positive[i]] != DS_TRUE;
    for (size_t i = 0; i < hazards; ++i) {
        if (result->observation_masks[hazard[i]] & (1U << DS_TRUE)) return DS_MODEL_BLOCK;
        incomplete |= result->observations[hazard[i]] != DS_FALSE;
    }
    return incomplete ? DS_MODEL_REVIEW : DS_MODEL_OBSERVED;
}
/** @brief Require explicit consistent evidence in every dimension without selecting a method or assuming access. */
static ds_model_state method_scope(const ds_assessment *assessment)
{
    for (unsigned m = 0; m < DS_METHOD_COUNT; ++m) for (unsigned d = 0; d < DS_CAP_DIMENSION_COUNT; ++d) {
        bool present = false;
        for (size_t i = 0; i < assessment->capability_count; ++i) {
            const ds_cap_evidence *e = &assessment->capabilities[i];
            if ((unsigned)e->method != m || (unsigned)e->dimension != d) continue;
            if (e->state != DS_CAP_SUPPORTED) return DS_MODEL_REVIEW;
            present = true;
        }
        if (!present) return DS_MODEL_REVIEW;
    }
    return DS_MODEL_OBSERVED;
}
/** @brief Copy validated observations and reconstruct findings rather than dereferencing caller-derived policy labels. */
static int copy_evidence(const ds_assessment *input, ds_plan *result)
{
    if (!input || input == &result->evidence || input->failed ||
        input->status < DS_INSPECTION_NO_BLOCKERS || input->status > DS_INSPECTION_BLOCKED ||
        input->check_count > DS_ASSESS_LIMIT || input->capability_count > DS_ASSESS_LIMIT ||
        input->finding_count > DS_ASSESS_LIMIT || !ds_probe_report_valid(&input->probe)) return -1;
    for (size_t i = 0; i < input->check_count; ++i) {
        const ds_check_evidence *e = &input->checks[i];
        if (ds_assessment_observe(&result->evidence, e->check, e->state, e->source, e->detail)) return -1;
    }
    for (size_t i = 0; i < input->capability_count; ++i) {
        const ds_cap_evidence *e = &input->capabilities[i];
        if (ds_assessment_capability(&result->evidence, e->method, e->dimension, e->state, e->source, e->detail)) return -1;
    }
    result->evidence.probe = input->probe;
    if (ds_assessment_evaluate(&result->evidence) || result->evidence.status != input->status) return -1;
    return 0;
}
/** @brief Derive only non-executable prerequisites from bounded fresh observations and the selected snapshot. */
int ds_plan_build(const ds_device *device, const ds_assessment *assessment, ds_plan *result)
{
    if (assessment == &result->evidence) return failed(result);
    ds_plan_init(result);
    if (!device || !bounded(device->path.value) || !bounded(device->device_id.value) ||
        !bounded(device->role) || !bounded(device->transport) || !bounded(device->read_only.source) ||
        copy_evidence(assessment, result)) return failed(result);
    /* Snapshot hazards cannot be hidden by an optimistic or incomplete supplied assessment. */
    if (ds_assessment_seed(&result->evidence, device) || ds_assessment_evaluate(&result->evidence)) return failed(result);
    summarize(result);
    result->target_observed = device->path.value && *device->path.value;
    result->prerequisites[DS_PLAN_TARGET] = result->target_observed ? DS_MODEL_OBSERVED : DS_MODEL_REVIEW;
    bool identity = result->observations[DS_CHECK_IDENTITY] == DS_TRUE &&
                    result->observations[DS_CHECK_GENERATION] == DS_TRUE &&
                    device->device_id.value && *device->device_id.value;
    result->prerequisites[DS_PLAN_IDENTITY] = identity ? DS_MODEL_OBSERVED : DS_MODEL_REVIEW;
    if (result->observation_masks[DS_CHECK_IDENTITY] & (1U << DS_FALSE)) result->prerequisites[DS_PLAN_IDENTITY] = DS_MODEL_BLOCK;
    static const ds_check usage[] = {DS_CHECK_USAGE, DS_CHECK_HOST_SCOPE, DS_CHECK_COLLECTION};
    static const ds_check consumers[] = {DS_CHECK_MOUNTED, DS_CHECK_SWAP, DS_CHECK_SYSTEM, DS_CHECK_READ_ONLY};
    static const ds_check topology[] = {DS_CHECK_TOPOLOGY, DS_CHECK_PHYSICAL_SCOPE};
    static const ds_check layers[] = {DS_CHECK_HOLDERS, DS_CHECK_SLAVES, DS_CHECK_LOGICAL};
    result->prerequisites[DS_PLAN_USAGE] = check_scope(result, usage, 3U, consumers, 4U);
    result->prerequisites[DS_PLAN_TOPOLOGY] = check_scope(result, topology, 2U, layers, 3U);
    if (result->prerequisites[DS_PLAN_TOPOLOGY] != DS_MODEL_BLOCK && result->observations[DS_CHECK_USB_BRIDGE] != DS_FALSE)
        result->prerequisites[DS_PLAN_TOPOLOGY] = DS_MODEL_REVIEW;
    result->prerequisites[DS_PLAN_METHOD_EVIDENCE] = method_scope(&result->evidence);
    for (unsigned r = DS_PLAN_PRIVILEGE; r < DS_PLAN_REQUIREMENT_COUNT; ++r) result->prerequisites[r] = DS_MODEL_UNIMPLEMENTED;
    result->status = DS_PLAN_INSPECTION_ONLY;
    for (unsigned r = 0; r <= DS_PLAN_METHOD_EVIDENCE; ++r)
        if (result->prerequisites[r] == DS_MODEL_REVIEW) result->status = DS_PLAN_REVIEW;
    if (result->evidence.status == DS_INSPECTION_REVIEW) result->status = DS_PLAN_REVIEW;
    if (!result->target_observed || !identity) result->status = DS_PLAN_INSUFFICIENT;
    if (result->evidence.status == DS_INSPECTION_BLOCKED) result->status = DS_PLAN_BLOCKED;
    return 0;
}
/** @brief Map only valid diagnosis states to stable non-authorizing labels. */
const char *ds_plan_state_name(ds_plan_state state)
{
    static const char *const labels[] = {"not_evaluated", "inspection_only", "review_required", "insufficient_evidence", "blocked"};
    return (unsigned)state < sizeof(labels) / sizeof(labels[0]) ? labels[state] : "invalid";
}
/** @brief Keep modeled observations distinct from real storage-operation outcomes. */
const char *ds_model_state_name(ds_model_state state)
{
    static const char *const labels[] = {"unknown_requires_review", "inspection_step_modeled", "would_block", "not_implemented"};
    return (unsigned)state < sizeof(labels) / sizeof(labels[0]) ? labels[state] : "invalid";
}
/** @brief Return a static prerequisite code without accepting arbitrary requests. */
const char *ds_plan_requirement_code(ds_plan_requirement value)
{
    return (unsigned)value < DS_PLAN_REQUIREMENT_COUNT ? requirements[value].code : "invalid";
}
/** @brief Return a static explanation without accepting caller-provided operation text. */
const char *ds_plan_requirement_detail(ds_plan_requirement value)
{
    return (unsigned)value < DS_PLAN_REQUIREMENT_COUNT ? requirements[value].detail : "invalid";
}
/** @brief Return an explicit evidence scope or future-boundary reference. */
const char *ds_plan_requirement_scope(ds_plan_requirement value)
{
    return (unsigned)value < DS_PLAN_REQUIREMENT_COUNT ? requirements[value].scope : "invalid";
}
