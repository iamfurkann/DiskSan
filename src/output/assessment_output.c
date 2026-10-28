/**
 * @file
 * @brief Render inspection evidence without deriving destructive permission from observations.
 */
#include "disksan/assessment.h"
#include "disksan/version.h"
#include "disksan/protocol.h"

static const char *const dimensions[] = {"support", "os_access", "applicability", "passthrough"};
/** @brief Convert enum truth to a stable explicit string instead of coercing unknown to false. */
static const char *truth(ds_truth state)
{
    return state == DS_TRUE ? "true" : state == DS_FALSE ? "false" : "unknown";
}
/** @brief Render only stable inspection states whose names do not imply permission. */
static const char *inspection(ds_inspection_status status)
{
    switch (status) {
        case DS_INSPECTION_BLOCKED: return "blocked";
        case DS_INSPECTION_REVIEW: return "review_required";
        case DS_INSPECTION_NO_BLOCKERS: return "no_blockers_observed_in_snapshot";
        default: return "not_evaluated";
    }
}
/** @brief Preserve capability uncertainty and distinguish unavailable queries from unsupported methods. */
static const char *cap_state(ds_cap_state state)
{
    switch (state) {
        case DS_CAP_SUPPORTED: return "observed_supported";
        case DS_CAP_UNSUPPORTED: return "observed_unsupported";
        case DS_CAP_UNAVAILABLE: return "unavailable";
        default: return "unknown";
    }
}
/** @brief Reduce independent method evidence; missing or contradictory information remains unknown. */
static ds_cap_state cap_summary(const ds_assessment *a, ds_method method, ds_cap_dimension dimension)
{
    bool yes = false, no = false, unknown = false, unavailable = false;
    for (size_t i = 0; i < a->capability_count; ++i) {
        const ds_cap_evidence *e = &a->capabilities[i];
        if (e->method != method || e->dimension != dimension) continue;
        yes |= e->state == DS_CAP_SUPPORTED; no |= e->state == DS_CAP_UNSUPPORTED;
        unknown |= e->state == DS_CAP_UNKNOWN; unavailable |= e->state == DS_CAP_UNAVAILABLE;
    }
    if (!yes && !no && unavailable && !unknown) return DS_CAP_UNAVAILABLE;
    if (unknown || unavailable || yes == no) return DS_CAP_UNKNOWN;
    return yes ? DS_CAP_SUPPORTED : DS_CAP_UNSUPPORTED;
}
/** @brief Require complete positive identity consensus for the snapshot-only verification label. */
static bool verified(const ds_assessment *a)
{
    bool yes = false;
    for (size_t i = 0; i < a->check_count; ++i) if (a->checks[i].check == DS_CHECK_IDENTITY) {
        if (a->checks[i].state != DS_TRUE) return false;
        yes = true;
    }
    return yes;
}
/** @brief Map fixed policy severity to an explanatory machine-readable string. */
static const char *severity(ds_severity value)
{
    return value == DS_SEVERITY_BLOCK ? "block" : value == DS_SEVERITY_REVIEW ? "review" : "info";
}
/** @brief Emit a JSON key/value pair using the shared UTF-8/control-safe encoder. */
static void field(FILE *out, const char *key, const char *value)
{
    ds_json_string(out, key); fputc(':', out); ds_json_string(out, value);
}
/** @brief Preserve every bounded original capability observation under its method summary. */
static void json_capabilities(FILE *out, const ds_assessment *a)
{
    fputc('[', out);
    for (unsigned m = 0; m < DS_METHOD_COUNT; ++m) {
        if (m) fputc(',', out);
        fputc('{', out); field(out, "method", ds_method_name((ds_method)m));
        for (unsigned d = 0; d < DS_CAP_DIMENSION_COUNT; ++d) {
            fputc(',', out); field(out, dimensions[d], cap_state(cap_summary(a, (ds_method)m, (ds_cap_dimension)d)));
        }
        fputs(",\"evidence\":[", out); bool comma = false;
        for (size_t i = 0; i < a->capability_count; ++i) {
            const ds_cap_evidence *e = &a->capabilities[i];
            if ((unsigned)e->method != m) continue;
            if (comma) fputc(',', out);
            comma = true;
            fputc('{', out); field(out, "dimension", (unsigned)e->dimension < DS_CAP_DIMENSION_COUNT ? dimensions[e->dimension] : "unknown");
            fputc(',', out); field(out, "state", cap_state(e->state));
            fputc(',', out); field(out, "source", e->source);
            fputc(',', out); field(out, "detail", e->detail); fputc('}', out);
        }
        fputs("]}", out);
    }
    fputc(']', out);
}
/** @brief Emit a separate schema-2 operation envelope, keeping inventory device fields intact. */
static void json_assessment(FILE *out, const ds_device *device, const ds_assessment *a, bool capabilities)
{
    fputs("{\"schema_version\":2,\"application\":\"DiskSan\",\"version\":", out); ds_json_string(out, DISKSAN_VERSION);
    fputs(",\"mode\":\"read-only\",\"snapshot\":true,", out);
    field(out, "operation", capabilities ? "capabilities" : "assess"); fputc(',', out);
    field(out, "assessment_kind", capabilities ? "capability-evidence" : "target-preflight");
    fputs(",\"device\":", out); ds_json_device(out, device);
    fputs(",\"identity\":{", out); field(out, "status", verified(a) ? "verified_for_snapshot" : "partially_verified");
    fputs(",\"guarantees_future_identity\":false,\"atomic\":false,\"valid_until\":\"end_of_observation\"},", out);
    field(out, "inspection_status", inspection(a->status));
    fputs(",\"erase_authorized\":false", out);
    if (a->probe.status != DS_PROBE_NOT_REQUESTED) {
        fputs(",\"probe\":{\"requested\":true,\"performed\":", out);
        fputs(a->probe.performed ? "true," : "false,", out);
        field(out, "query", "nvme_identify_controller"); fputc(',', out);
        field(out, "provider", ds_probe_provider_name(a->probe.provider)); fputc(',', out);
        field(out, "status", ds_probe_status_name(a->probe.status)); fputc(',', out);
        field(out, "reason", ds_probe_reason_name(a->probe.reason)); fputs("}", out);
    }
    fputs(",\"observations\":[", out);
    for (size_t i = 0; i < a->check_count; ++i) {
        const ds_check_evidence *e = &a->checks[i];
        if (i) fputc(',', out);
        fputc('{', out); field(out, "check", ds_check_name(e->check));
        fputc(',', out); field(out, "state", truth(e->state));
        fputc(',', out); field(out, "source", e->source);
        fputc(',', out); field(out, "detail", e->detail); fputc('}', out);
    }
    fputs("],\"findings\":[", out);
    for (size_t i = 0; i < a->finding_count; ++i) {
        const ds_finding *f = &a->findings[i];
        if (i) fputc(',', out);
        fputc('{', out); field(out, "code", f->code);
        fputc(',', out); field(out, "category", f->category);
        fputc(',', out); field(out, "subject", f->subject);
        fputc(',', out); field(out, "severity", severity(f->severity));
        fputc(',', out); field(out, "state", truth(f->state));
        fputc(',', out); field(out, "source", "policy:observed_evidence");
        fputc(',', out); field(out, "message", f->message); fputc('}', out);
    }
    fputs("],\"capabilities\":", out); json_capabilities(out, a); fputs("}\n", out);
}
/** @brief Write untrusted metadata through the shared terminal escape routine. */
static void text_value(FILE *out, const char *value)
{
    ds_terminal_string(out, value, DS_TEXT_LIMIT * 4U);
}
/** @brief Explain derived findings separately from their original observed evidence. */
static void text_assessment(FILE *out, const ds_device *device, const ds_assessment *a, bool capabilities)
{
    fprintf(out, "DiskSan v%s - %s (read-only snapshot)\n", DISKSAN_VERSION, capabilities ? "Capability Evidence" : "Target Assessment");
    fputs("Target: ", out); text_value(out, device->path.value); fputc('\n', out);
    fprintf(out, "Inspection status: %s\nIdentity: %s; non-atomic; no future guarantee.\n", inspection(a->status), verified(a) ? "verified for this snapshot" : "partially verified");
    fputs("Erase authorized: NO. This assessment expires at the end of observation.\n", out);
    if (a->probe.status != DS_PROBE_NOT_REQUESTED)
        fprintf(out, "Protocol probe: %s; provider=%s; performed=%s; reason=%s.\n%s\n",
                ds_probe_status_name(a->probe.status), ds_probe_provider_name(a->probe.provider),
                a->probe.performed ? "yes" : "no", ds_probe_reason_name(a->probe.reason), ds_probe_reason_detail(a->probe.reason));
    fputs("Findings:\n", out);
    for (size_t i = 0; i < a->finding_count; ++i) {
        const ds_finding *f = &a->findings[i];
        fprintf(out, "  [%s] %s (%s): ", severity(f->severity), f->code, f->subject);
        text_value(out, f->message); fputc('\n', out);
    }
    fputs("Observed evidence:\n", out);
    for (size_t i = 0; i < a->check_count; ++i) {
        const ds_check_evidence *e = &a->checks[i];
        fprintf(out, "  %s=%s [", ds_check_name(e->check), truth(e->state)); text_value(out, e->source);
        fputs("]: ", out); text_value(out, e->detail); fputc('\n', out);
    }
    fputs(a->probe.status == DS_PROBE_NOT_REQUESTED ? "Informational method evidence (no commands are issued):\n" :
          "Informational method evidence (no sanitization method was executed):\n", out);
    for (unsigned m = 0; m < DS_METHOD_COUNT; ++m) {
        fprintf(out, "  %s:", ds_method_name((ds_method)m));
        for (unsigned d = 0; d < DS_CAP_DIMENSION_COUNT; ++d) fprintf(out, " %s=%s", dimensions[d], cap_state(cap_summary(a, (ds_method)m, (ds_cap_dimension)d)));
        fputc('\n', out);
        for (size_t i = 0; i < a->capability_count; ++i) {
            const ds_cap_evidence *e = &a->capabilities[i];
            if ((unsigned)e->method != m) continue;
            fprintf(out, "    %s=%s [", (unsigned)e->dimension < DS_CAP_DIMENSION_COUNT ? dimensions[e->dimension] : "unknown", cap_state(e->state));
            text_value(out, e->source); fputs("]: ", out); text_value(out, e->detail); fputc('\n', out);
        }
    }
}
/** @brief Refuse partial/poisoned results before writing any bytes and propagate stream failure. */
int ds_assessment_output(FILE *out, const ds_device *device, const ds_assessment *result, bool json, bool capabilities)
{
    if (result->failed || result->status == DS_INSPECTION_NOT_EVALUATED || result->check_count > DS_ASSESS_LIMIT ||
        result->capability_count > DS_ASSESS_LIMIT || result->finding_count > DS_ASSESS_LIMIT ||
        !ds_probe_report_valid(&result->probe) || (!capabilities && result->probe.status != DS_PROBE_NOT_REQUESTED)) return 1;
    if (json) json_assessment(out, device, result, capabilities);
    else text_assessment(out, device, result, capabilities);
    int rc = fflush(out);
    return rc == 0 && !ferror(out) ? 0 : 1;
}
