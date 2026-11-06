/** @file
 * @brief Render bounded hypothetical prerequisites separately from observed facts without operational claims.
 */
#include "disksan/plan.h"
#include "disksan/version.h"

/** @brief Fixed bounded transcript vocabulary; these descriptions cannot be submitted as device commands. */
static const char *const step_codes[DS_PLAN_REQUIREMENT_COUNT] = {
    "observe_target", "compare_identity", "review_usage", "review_topology", "review_method_evidence",
    "privilege_boundary_unimplemented", "authorization_boundary_unimplemented", "exclusive_scope_unimplemented",
    "operation_backend_unavailable", "completion_verifier_unavailable"
};

/** @brief Escape both JSON keys and values through the unchanged schema-2 encoder. */
static void field(FILE *out, const char *key, const char *value)
{
    ds_json_string(out, key); fputc(':', out); ds_json_string(out, value);
}
/** @brief Preserve tri-state observation values rather than coercing unknown to false. */
static const char *truth(ds_truth state)
{
    return state == DS_TRUE ? "true" : state == DS_FALSE ? "false" : "unknown";
}
/** @brief Preserve unavailable and unsupported as distinct reported capability evidence. */
static const char *capability(ds_cap_state state)
{
    switch (state) {
        case DS_CAP_SUPPORTED: return "observed_supported";
        case DS_CAP_UNSUPPORTED: return "observed_unsupported";
        case DS_CAP_UNAVAILABLE: return "unavailable";
        default: return "unknown";
    }
}
/** @brief Report stable policy severity without converting it into OS access or operation status. */
static const char *severity(ds_severity state)
{
    return state == DS_SEVERITY_BLOCK ? "block" : state == DS_SEVERITY_REVIEW ? "review" : "info";
}
/** @brief Output only reconstructed bounded findings, never caller-provided derived pointers. */
static void findings(FILE *out, const ds_assessment *a)
{
    fputc('[', out);
    for (size_t i = 0; i < a->finding_count; ++i) {
        const ds_finding *f = &a->findings[i];
        if (i) fputc(',', out);
        fputc('{', out); field(out, "code", f->code); fputc(',', out);
        field(out, "category", f->category); fputc(',', out); field(out, "subject", f->subject); fputc(',', out);
        field(out, "severity", severity(f->severity)); fputc(',', out); field(out, "state", truth(f->state)); fputc(',', out);
        field(out, "source", "policy:observed_evidence"); fputc(',', out); field(out, "message", f->message); fputc('}', out);
    }
    fputc(']', out);
}
/** @brief Preserve every bounded source observation and independent capability dimension. */
static void evidence(FILE *out, const ds_assessment *a)
{
    fputs(",\"observations\":[", out);
    for (size_t i = 0; i < a->check_count; ++i) {
        const ds_check_evidence *e = &a->checks[i];
        if (i) fputc(',', out);
        fputc('{', out); field(out, "check", ds_check_name(e->check)); fputc(',', out);
        field(out, "state", truth(e->state)); fputc(',', out); field(out, "source", e->source); fputc(',', out);
        field(out, "detail", e->detail); fputc('}', out);
    }
    fputs("],\"method_evidence\":[", out);
    static const char *const dimensions[] = {"support", "os_access", "applicability", "passthrough"};
    for (size_t i = 0; i < a->capability_count; ++i) {
        const ds_cap_evidence *e = &a->capabilities[i];
        if (i) fputc(',', out);
        fputc('{', out); field(out, "method", ds_method_name(e->method)); fputc(',', out);
        field(out, "dimension", dimensions[e->dimension]); fputc(',', out); field(out, "state", capability(e->state)); fputc(',', out);
        field(out, "source", e->source); fputc(',', out); field(out, "detail", e->detail); fputc('}', out);
    }
    fputc(']', out);
}
/** @brief Serialize a distinct plan envelope with the unchanged single-device representation. */
static void json_plan(FILE *out, const ds_device *device, const ds_plan *plan, ds_plan_origin origin, bool simulation)
{
    fputs("{\"schema_version\":2,\"application\":\"DiskSan\",\"version\":", out); ds_json_string(out, DISKSAN_VERSION);
    fputc(',', out); field(out, "operation", simulation ? "simulate" : "plan");
    fputs(",\"mode\":\"read-only\",\"simulation_only\":true,\"execution_available\":false,\"erase_authorized\":false", out);
    fputs(",\"verification_available\":false,\"privilege_unknown\":true,\"physical_scope_unknown\":", out);
    fputs(plan->observations[DS_CHECK_PHYSICAL_SCOPE] == DS_TRUE ? "false" : "true", out);
    fputs(",\"target_observed\":", out); fputs(plan->target_observed ? "true" : "false", out);
    fputc(',', out); field(out, "inspection_status", ds_plan_state_name(plan->status)); fputc(',', out);
    field(out, "evidence_origin", origin == DS_PLAN_SYNTHETIC_OBSERVATIONS ? "synthetic_fixture" : "native_read_only_snapshot");
    fputs(",\"identity\":{\"atomic\":false,\"guarantees_future_identity\":false,\"valid_until\":\"end_of_observation\",", out);
    field(out, "status", plan->prerequisites[DS_PLAN_IDENTITY] == DS_MODEL_OBSERVED ? "observed_for_snapshot" : "incomplete");
    fputs("},\"device\":", out); ds_json_device(out, device);
    fputs(",\"findings\":", out); findings(out, &plan->evidence); evidence(out, &plan->evidence);
    fputs(",\"prerequisites\":[", out);
    for (unsigned i = 0; i < DS_PLAN_REQUIREMENT_COUNT; ++i) {
        if (i) fputc(',', out);
        fputs("{\"modeled_only\":true,", out); field(out, "code", ds_plan_requirement_code((ds_plan_requirement)i)); fputc(',', out);
        field(out, "state", ds_model_state_name(plan->prerequisites[i])); fputc(',', out);
        field(out, "evidence_scope", ds_plan_requirement_scope((ds_plan_requirement)i)); fputc(',', out);
        field(out, "explanation", ds_plan_requirement_detail((ds_plan_requirement)i)); fputc('}', out);
    }
    fputs("],\"steps\":[", out);
    if (simulation) for (unsigned i = 0; i < DS_PLAN_REQUIREMENT_COUNT; ++i) {
        if (i) fputc(',', out);
        fprintf(out, "{\"index\":%u,\"modeled_only\":true,", i + 1U);
        field(out, "code", step_codes[i]); fputc(',', out);
        field(out, "state", ds_model_state_name(plan->prerequisites[i])); fputc(',', out);
        field(out, "prerequisite", ds_plan_requirement_code((ds_plan_requirement)i)); fputc(',', out);
        field(out, "evidence_scope", ds_plan_requirement_scope((ds_plan_requirement)i)); fputc(',', out);
        field(out, "explanation", ds_plan_requirement_detail((ds_plan_requirement)i)); fputc('}', out);
    }
    fputc(']', out);
    if (simulation) { fputc(',', out); field(out, "simulation_result", "non_executable_model_only"); }
    fputs("}\n", out);
}
/** @brief Explain hypothetical requirements in escaped terminal output with a permanent no-erasure label. */
static void text_plan(FILE *out, const ds_device *device, const ds_plan *plan, ds_plan_origin origin, bool simulation)
{
    fprintf(out, "DiskSan v%s - %s\nSIMULATION ONLY - DOES NOT ERASE DATA\n", DISKSAN_VERSION,
            simulation ? "Non-executable simulation" : "Inspection plan");
    fputs(origin == DS_PLAN_SYNTHETIC_OBSERVATIONS ? "DEMO DATA - SYNTHETIC FIXTURE\n" : "Observed source: native read-only snapshot.\n", out);
    fputs("Target: ", out); ds_terminal_string(out, device->path.value, DS_TEXT_LIMIT * 4U); fputc('\n', out);
    fprintf(out, "Inspection status: %s\nExecution available: NO. Erase authorized: NO. Verification available: NO.\n", ds_plan_state_name(plan->status));
    fputs("Identity is non-atomic, has no future guarantee and expires at the end of observation.\nObserved policy findings:\n", out);
    for (size_t i = 0; i < plan->evidence.finding_count; ++i) {
        const ds_finding *f = &plan->evidence.findings[i];
        fprintf(out, "  [%s] %s (%s): ", severity(f->severity), f->code, f->subject);
        ds_terminal_string(out, f->message, DS_TEXT_LIMIT); fputc('\n', out);
    }
    fputs("Hypothetical prerequisites (no method selected):\n", out);
    for (unsigned i = 0; i < DS_PLAN_REQUIREMENT_COUNT; ++i)
        fprintf(out, "  %s: %s [%s]\n    %s\n", ds_plan_requirement_code((ds_plan_requirement)i),
                ds_model_state_name(plan->prerequisites[i]), ds_plan_requirement_scope((ds_plan_requirement)i),
                ds_plan_requirement_detail((ds_plan_requirement)i));
    if (simulation) {
        fputs("Deterministic model transcript (no operation was attempted):\n", out);
        for (unsigned i = 0; i < DS_PLAN_REQUIREMENT_COUNT; ++i)
            fprintf(out, "  %u. %s: %s (modeled only)\n", i + 1U, step_codes[i], ds_model_state_name(plan->prerequisites[i]));
        fputs("Simulation result: non_executable_model_only. No media effect or certificate.\n", out);
    }
}
/** @brief Reconstruct and compare the diagnosis before rendering any byte; failed or partial results cannot be published. */
static int output(FILE *out, const ds_device *device, const ds_plan *plan, bool json, ds_plan_origin origin, bool simulation)
{
    if (!plan || plan->failed || (unsigned)origin > DS_PLAN_SYNTHETIC_OBSERVATIONS) return 1;
    ds_plan checked;
    if (ds_plan_build(device, &plan->evidence, &checked) || checked.status != plan->status ||
        checked.target_observed != plan->target_observed) return 1;
    for (unsigned i = 0; i < DS_CHECK_COUNT; ++i)
        if (checked.observations[i] != plan->observations[i] || checked.observation_masks[i] != plan->observation_masks[i]) return 1;
    for (unsigned i = 0; i < DS_PLAN_REQUIREMENT_COUNT; ++i) if (checked.prerequisites[i] != plan->prerequisites[i]) return 1;
    if (json) json_plan(out, device, &checked, origin, simulation);
    else text_plan(out, device, &checked, origin, simulation);
    int rc = fflush(out);
    return rc == 0 && !ferror(out) ? 0 : 1;
}
/** @brief Publish only checked non-executable prerequisites without a transcript. */
int ds_plan_output(FILE *out, const ds_device *device, const ds_plan *plan, bool json, ds_plan_origin origin)
{
    return output(out, device, plan, json, origin, false);
}
/** @brief Publish only fixed modeled steps; this path cannot perform or authorize an operation. */
int ds_simulation_output(FILE *out, const ds_device *device, const ds_plan *plan, bool json, ds_plan_origin origin)
{
    return output(out, device, plan, json, origin, true);
}
