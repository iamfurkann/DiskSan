/**
 * @file
 * @brief Evaluate bounded observational evidence with no OS calls or device operations.
 */
#include "disksan/assessment.h"
#include <stdlib.h>
#include <string.h>

/** @brief Describe a stable dimension and the meaning of a true observation. */
typedef struct {
    const char *key;
    const char *category;
    const char *code;
    const char *unknown_code;
    const char *message;
    bool hazard;
    ds_severity severity;
} rule;

static const rule rules[DS_CHECK_COUNT] = {
    {"identity", "identity", "identity_unverified", "identity_unverified", "Current target identity is not fully verified.", false, DS_SEVERITY_REVIEW},
    {"generation", "identity", "generation_unverified", "generation_unverified", "Device-number or path reuse cannot be excluded.", false, DS_SEVERITY_REVIEW},
    {"topology", "topology", "topology_incomplete", "topology_unverified", "The storage dependency graph is incomplete or unverified.", false, DS_SEVERITY_REVIEW},
    {"usage", "usage", "usage_incomplete", "usage_unverified", "Observed usage does not establish absence of other consumers.", false, DS_SEVERITY_REVIEW},
    {"host_scope", "platform_limit", "host_scope_incomplete", "host_scope_unverified", "The observation scope may omit other namespaces or sessions.", false, DS_SEVERITY_REVIEW},
    {"physical_scope", "topology", "physical_scope_incomplete", "physical_scope_unverified", "Physical media and controller or namespace scope are not fully established.", false, DS_SEVERITY_REVIEW},
    {"mounted", "usage", "mounted_dependency", "mount_state_unverified", "A selected target or related device has a mounted consumer.", true, DS_SEVERITY_BLOCK},
    {"swap", "usage", "active_swap_dependency", "swap_state_unverified", "A selected target or related device backs active swap.", true, DS_SEVERITY_BLOCK},
    {"system", "usage", "system_dependency", "system_state_unverified", "The target participates in an observed root, boot or system dependency.", true, DS_SEVERITY_BLOCK},
    {"read_only", "usage", "device_read_only", "read_only_state_unverified", "The device reports a read-only flag; this is not a capability query.", true, DS_SEVERITY_BLOCK},
    {"holders", "topology", "active_holder", "holder_state_unverified", "A holder consumes the selected device.", true, DS_SEVERITY_BLOCK},
    {"slaves", "topology", "layered_backing", "backing_state_unverified", "Backing relationships expose a layered target.", true, DS_SEVERITY_BLOCK},
    {"logical", "role", "logical_target", "target_role_unverified", "A logical layer is not a whole physical-media target.", true, DS_SEVERITY_BLOCK},
    {"usb_bridge", "topology", "bridge_scope_unverified", "bridge_state_unverified", "Bridge passthrough and physical scope are not established by bus classification.", true, DS_SEVERITY_REVIEW},
    {"collection", "observation_error", "partial_observation", "collection_unverified", "One or more native observations are incomplete or unavailable.", false, DS_SEVERITY_REVIEW}
};
static const char *const methods[DS_METHOD_COUNT] = {
    "nvme_sanitize_block_erase", "nvme_sanitize_crypto_erase", "nvme_sanitize_overwrite",
    "ata_security_erase", "ata_sanitize", "scsi_sanitize"
};

/** @brief Return a stable dimension key without indexing outside the rule table. */
const char *ds_check_name(ds_check check)
{
    return (unsigned)check < DS_CHECK_COUNT ? rules[check].key : "invalid";
}
/** @brief Return a stable method key without indexing outside the name table. */
const char *ds_method_name(ds_method method)
{
    return (unsigned)method < DS_METHOD_COUNT ? methods[method] : "invalid";
}
/** @brief Clear all inline evidence and invalidate prior derived findings. */
void ds_assessment_init(ds_assessment *result)
{
    memset(result, 0, sizeof(*result));
}
/** @brief Refuse silent truncation and avoid unbounded string scans. */
static bool fits(const char *text, size_t capacity)
{
    if (!text || !*text) return false;
    for (size_t i = 1; i < capacity; ++i) if (!text[i]) return true;
    return false;
}
/** @brief Poison partial evidence so subsequent callers cannot publish a positive result. */
static int invalid(ds_assessment *result)
{
    result->failed = true;
    result->status = DS_INSPECTION_NOT_EVALUATED;
    result->finding_count = 0;
    return -1;
}
/** @brief Copy bounded check evidence and preserve conflicting observations separately. */
int ds_assessment_observe(ds_assessment *result, ds_check check, ds_truth state, const char *source, const char *detail)
{
    if (result->failed || result->check_count > DS_ASSESS_LIMIT ||
        (unsigned)check >= DS_CHECK_COUNT || (unsigned)state > DS_TRUE ||
        !fits(source, DS_ASSESS_SOURCE) || !fits(detail, DS_ASSESS_DETAIL)) return invalid(result);
    for (size_t i = 0; i < result->check_count; ++i) {
        const ds_check_evidence *e = &result->checks[i];
        if (e->check == check && e->state == state && !strcmp(e->source, source) && !strcmp(e->detail, detail)) return 0;
    }
    if (result->check_count == DS_ASSESS_LIMIT) return invalid(result);
    ds_check_evidence *e = &result->checks[result->check_count++];
    e->check = check; e->state = state;
    memcpy(e->source, source, strlen(source) + 1U);
    memcpy(e->detail, detail, strlen(detail) + 1U);
    result->status = DS_INSPECTION_NOT_EVALUATED; result->finding_count = 0;
    return 0;
}
/** @brief Copy independent capability evidence without converting privilege into support. */
int ds_assessment_capability(ds_assessment *result, ds_method method, ds_cap_dimension dimension, ds_cap_state state, const char *source, const char *detail)
{
    if (result->failed || result->capability_count > DS_ASSESS_LIMIT ||
        (unsigned)method >= DS_METHOD_COUNT || (unsigned)dimension >= DS_CAP_DIMENSION_COUNT ||
        (unsigned)state > DS_CAP_UNAVAILABLE || !fits(source, DS_ASSESS_SOURCE) ||
        !fits(detail, DS_ASSESS_DETAIL)) return invalid(result);
    for (size_t i = 0; i < result->capability_count; ++i) {
        const ds_cap_evidence *e = &result->capabilities[i];
        if (e->method == method && e->dimension == dimension && e->state == state &&
            !strcmp(e->source, source) && !strcmp(e->detail, detail)) return 0;
    }
    if (result->capability_count == DS_ASSESS_LIMIT) return invalid(result);
    ds_cap_evidence *e = &result->capabilities[result->capability_count++];
    e->method = method; e->dimension = dimension; e->state = state;
    memcpy(e->source, source, strlen(source) + 1U);
    memcpy(e->detail, detail, strlen(detail) + 1U);
    result->status = DS_INSPECTION_NOT_EVALUATED; result->finding_count = 0;
    return 0;
}
/** @brief Sort check evidence by stable keys, provenance and content independent of arrival. */
static int compare_check(const void *left, const void *right)
{
    const ds_check_evidence *a = left, *b = right;
    if (a->check != b->check) return a->check < b->check ? -1 : 1;
    int n = strcmp(a->source, b->source);
    if (!n) n = strcmp(a->detail, b->detail);
    if (!n && a->state != b->state) n = a->state < b->state ? -1 : 1;
    return n;
}
/** @brief Sort method observations without losing conflicting claims. */
static int compare_capability(const void *left, const void *right)
{
    const ds_cap_evidence *a = left, *b = right;
    if (a->method != b->method) return a->method < b->method ? -1 : 1;
    if (a->dimension != b->dimension) return a->dimension < b->dimension ? -1 : 1;
    int n = strcmp(a->source, b->source);
    if (!n) n = strcmp(a->detail, b->detail);
    if (!n && a->state != b->state) n = a->state < b->state ? -1 : 1;
    return n;
}
/** @brief Append one static derived finding and monotonically increase content severity. */
static int finding(ds_assessment *result, const char *code, const char *category, const char *subject, const char *message, ds_severity severity, ds_truth state)
{
    if (result->finding_count == DS_ASSESS_LIMIT) return invalid(result);
    result->findings[result->finding_count++] = (ds_finding){code, category, subject, message, severity, state};
    if (severity == DS_SEVERITY_BLOCK) result->status = DS_INSPECTION_BLOCKED;
    else if (severity == DS_SEVERITY_REVIEW && result->status != DS_INSPECTION_BLOCKED) result->status = DS_INSPECTION_REVIEW;
    return 0;
}
/** @brief Reduce all independent observations to conservative deterministic findings. */
int ds_assessment_evaluate(ds_assessment *result)
{
    if (result->failed || result->check_count > DS_ASSESS_LIMIT || result->capability_count > DS_ASSESS_LIMIT) return invalid(result);
    result->finding_count = 0;
    result->status = DS_INSPECTION_NO_BLOCKERS;
    for (unsigned c = 0; c < DS_CHECK_COUNT; ++c) {
        bool yes = false, no = false, unknown = false;
        for (size_t i = 0; i < result->check_count; ++i) {
            const ds_check_evidence *e = &result->checks[i];
            if ((unsigned)e->check != c) continue;
            yes |= e->state == DS_TRUE; no |= e->state == DS_FALSE; unknown |= e->state == DS_UNKNOWN;
        }
        const rule *r = &rules[c];
        bool conflict = yes && no;
        ds_truth state = conflict || unknown || (!yes && !no) ? DS_UNKNOWN : yes ? DS_TRUE : DS_FALSE;
        if (conflict && finding(result, "conflicting_evidence", r->category, r->key,
                                "Contradictory observations are retained; no consensus is assumed.", DS_SEVERITY_REVIEW, DS_UNKNOWN)) return -1;
        if (r->hazard && yes) {
            if (finding(result, r->code, r->category, r->key, r->message, r->severity, state)) return -1;
        } else if ((!r->hazard && state != DS_TRUE) || (r->hazard && state == DS_UNKNOWN)) {
            if (finding(result, state == DS_UNKNOWN ? r->unknown_code : r->code, r->category, r->key,
                        state == DS_UNKNOWN ? "This dimension is unknown or incompletely observed; inspect the evidence and scope." : r->message,
                        DS_SEVERITY_REVIEW, state)) return -1;
        }
        if (c == DS_CHECK_IDENTITY && no && finding(result, "identity_changed", "identity", r->key,
                                                   "Contradictory target identity must fail target resolution.", DS_SEVERITY_BLOCK, DS_FALSE)) return -1;
    }
    for (unsigned m = 0; m < DS_METHOD_COUNT; ++m) {
        bool incomplete = false, conflict = false, unavailable = false, unsupported = false;
        for (unsigned d = 0; d < DS_CAP_DIMENSION_COUNT; ++d) {
            bool yes = false, no = false, unknown = false, missing = true;
            for (size_t i = 0; i < result->capability_count; ++i) {
                const ds_cap_evidence *e = &result->capabilities[i];
                if ((unsigned)e->method != m || (unsigned)e->dimension != d) continue;
                missing = false;
                yes |= e->state == DS_CAP_SUPPORTED; no |= e->state == DS_CAP_UNSUPPORTED;
                unknown |= e->state == DS_CAP_UNKNOWN || e->state == DS_CAP_UNAVAILABLE;
                unavailable |= e->state == DS_CAP_UNAVAILABLE;
            }
            conflict |= yes && no;
            incomplete |= missing || unknown || no;
            unsupported |= d == DS_CAP_SUPPORT && no;
        }
        if (conflict && finding(result, "capability_conflict", "capability", methods[m],
                                "Capability flags conflict; retained evidence cannot establish support.", DS_SEVERITY_REVIEW, DS_UNKNOWN)) return -1;
        if (incomplete || unavailable || unsupported) {
            const char *code = unavailable ? "capability_unavailable" : unsupported && !conflict ? "method_observed_unsupported" : "capability_unverified";
            if (finding(result, code, "capability", methods[m], "Support, access, applicability and passthrough are independent; no operation is authorized.", DS_SEVERITY_REVIEW, DS_UNKNOWN)) return -1;
        }
    }
    if (finding(result, "snapshot_not_authorization", "platform_limit", "snapshot",
                "Observations are non-atomic, expire at the end of inspection and never authorize erasure.", DS_SEVERITY_INFO, DS_TRUE)) return -1;
    qsort(result->checks, result->check_count, sizeof(result->checks[0]), compare_check);
    qsort(result->capabilities, result->capability_count, sizeof(result->capabilities[0]), compare_capability);
    return 0;
}
/** @brief Derive only facts directly established by existing classification and metadata. */
int ds_assessment_seed(ds_assessment *result, const ds_device *device)
{
    const char *role = device->role ? device->role : "unknown";
    ds_truth logical = DS_UNKNOWN;
    if (!strcmp(role, "disk") || !strcmp(role, "namespace")) logical = DS_FALSE;
    if (!strcmp(role, "partition") || !strcmp(role, "volume") || !strcmp(role, "snapshot") ||
        !strcmp(role, "virtual") || !strcmp(role, "composite")) logical = DS_TRUE;
    if (ds_assessment_observe(result, DS_CHECK_LOGICAL, logical, "discovery:role", role)) return -1;
    ds_truth read_only = device->read_only.known && device->read_only.value <= 1U ? (device->read_only.value ? DS_TRUE : DS_FALSE) : DS_UNKNOWN;
    if (ds_assessment_observe(result, DS_CHECK_READ_ONLY, read_only,
                              device->read_only.source ? device->read_only.source : "discovery:read_only", "Device flag; not application access or command support.")) return -1;
    if (device->holders.count && ds_assessment_observe(result, DS_CHECK_HOLDERS, DS_TRUE, "discovery:holders", "At least one holder was observed.")) return -1;
    if (device->slaves.count && ds_assessment_observe(result, DS_CHECK_SLAVES, DS_TRUE, "discovery:slaves", "At least one backing device was observed.")) return -1;
    if (device->transport && !strcmp(device->transport, "usb") &&
        ds_assessment_observe(result, DS_CHECK_USB_BRIDGE, DS_TRUE, "discovery:transport", "USB classification does not validate bridge passthrough.")) return -1;
    return 0;
}
