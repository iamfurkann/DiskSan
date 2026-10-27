/**
 * @file
 * @brief Exercise portable inspection policy with synthetic evidence and no native access.
 */
#include "disksan/assessment.h"
#include <stdlib.h>
#include <string.h>

static size_t checks;
/** @brief Count assertions and report only synthetic line diagnostics. */
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)

/** @brief Find a stable code/subject finding without depending on its array position. */
static const ds_finding *find(const ds_assessment *a, const char *code, const char *subject)
{
    for (size_t i = 0; i < a->finding_count; ++i)
        if (!strcmp(a->findings[i].code, code) && !strcmp(a->findings[i].subject, subject)) return &a->findings[i];
    return NULL;
}
/** @brief Fill a fully synthetic inspection baseline; it is never live-device evidence. */
static void complete(ds_assessment *a)
{
    ds_assessment_init(a);
    for (unsigned c = 0; c < DS_CHECK_COUNT; ++c) {
        bool hazard = c >= DS_CHECK_MOUNTED && c <= DS_CHECK_USB_BRIDGE;
        CHECK(ds_assessment_observe(a, (ds_check)c, hazard ? DS_FALSE : DS_TRUE, "synthetic:complete", "Fixture-only observation.") == 0);
    }
    for (unsigned m = 0; m < DS_METHOD_COUNT; ++m)
        for (unsigned d = 0; d < DS_CAP_DIMENSION_COUNT; ++d)
            CHECK(ds_assessment_capability(a, (ds_method)m, (ds_cap_dimension)d, DS_CAP_SUPPORTED, "synthetic:identify", "Fixture-only flags, not a hardware test.") == 0);
}
/** @brief Prove empty, partial and poisoned input cannot become a positive inspection. */
static void unknown_and_errors(void)
{
    ds_assessment a;
    ds_assessment_init(&a);
    CHECK(a.status == DS_INSPECTION_NOT_EVALUATED);
    CHECK(ds_assessment_evaluate(&a) == 0 && a.status == DS_INSPECTION_REVIEW);
    CHECK(find(&a, "usage_unverified", "usage") != NULL);
    CHECK(find(&a, "snapshot_not_authorization", "snapshot") != NULL);
    CHECK(ds_assessment_observe(&a, DS_CHECK_MOUNTED, DS_FALSE, "namespace", "No matching records here.") == 0);
    CHECK(ds_assessment_evaluate(&a) == 0 && a.status == DS_INSPECTION_REVIEW);
    CHECK(find(&a, "host_scope_unverified", "host_scope") != NULL);
    for (unsigned scenario = 0; scenario < 7; ++scenario) {
        complete(&a);
        char overlong[DS_ASSESS_DETAIL + 1U]; memset(overlong, 'X', sizeof(overlong)); overlong[sizeof(overlong) - 1U] = '\0';
        int rc;
        if (scenario == 0) rc = ds_assessment_observe(&a, DS_CHECK_COUNT, DS_TRUE, "test", "detail");
        else if (scenario == 1) rc = ds_assessment_observe(&a, DS_CHECK_MOUNTED, (ds_truth)-1, "test", "detail");
        else if (scenario == 2) rc = ds_assessment_observe(&a, DS_CHECK_MOUNTED, DS_FALSE, NULL, "detail");
        else if (scenario == 3) rc = ds_assessment_observe(&a, DS_CHECK_MOUNTED, DS_FALSE, "test", overlong);
        else if (scenario == 4) rc = ds_assessment_capability(&a, DS_METHOD_COUNT, DS_CAP_SUPPORT, DS_CAP_SUPPORTED, "test", "detail");
        else if (scenario == 5) rc = ds_assessment_capability(&a, DS_METHOD_NVME_BLOCK, DS_CAP_DIMENSION_COUNT, DS_CAP_SUPPORTED, "test", "detail");
        else rc = ds_assessment_capability(&a, DS_METHOD_NVME_BLOCK, DS_CAP_SUPPORT, (ds_cap_state)99, "test", "detail");
        CHECK(rc == -1 && a.failed);
        CHECK(ds_assessment_evaluate(&a) == -1 && a.status == DS_INSPECTION_NOT_EVALUATED && a.finding_count == 0);
        CHECK(ds_assessment_observe(&a, DS_CHECK_MOUNTED, DS_FALSE, "test", "later") == -1);
        ds_assessment_init(&a); ds_assessment_init(&a);
        CHECK(!a.failed && !a.check_count && !a.capability_count);
    }
}
/** @brief Known hazards stay blocked despite competing negative or incomplete observations. */
static void hazards_and_conflicts(void)
{
    const char *codes[] = {"mounted_dependency", "active_swap_dependency", "system_dependency", "device_read_only", "active_holder", "layered_backing", "logical_target"};
    for (unsigned c = DS_CHECK_MOUNTED; c <= DS_CHECK_LOGICAL; ++c) {
        ds_assessment a;
        complete(&a);
        CHECK(ds_assessment_observe(&a, (ds_check)c, DS_TRUE, "synthetic:hazard", "Observed consumer or logical layer.") == 0);
        CHECK(ds_assessment_observe(&a, (ds_check)c, DS_UNKNOWN, "synthetic:denied", "Permission denied to a second source.") == 0);
        CHECK(ds_assessment_evaluate(&a) == 0 && a.status == DS_INSPECTION_BLOCKED);
        const ds_finding *f = find(&a, codes[c - DS_CHECK_MOUNTED], ds_check_name((ds_check)c));
        CHECK(f && f->severity == DS_SEVERITY_BLOCK && f->state == DS_UNKNOWN);
        CHECK(find(&a, "conflicting_evidence", ds_check_name((ds_check)c)) != NULL);
    }
    ds_assessment a;
    complete(&a);
    CHECK(ds_assessment_observe(&a, DS_CHECK_IDENTITY, DS_FALSE, "synthetic:changed", "Generation changed.") == 0);
    CHECK(ds_assessment_evaluate(&a) == 0 && a.status == DS_INSPECTION_BLOCKED);
    CHECK(find(&a, "identity_changed", "identity") != NULL);
    complete(&a);
    CHECK(ds_assessment_observe(&a, DS_CHECK_MOUNTED, DS_UNKNOWN, "synthetic:partial", "Namespace not complete.") == 0);
    CHECK(ds_assessment_evaluate(&a) == 0 && a.status == DS_INSPECTION_REVIEW);
    CHECK(find(&a, "mount_state_unverified", "mounted") != NULL);
    /* Every subset of true/false/unknown keeps an observed hazard blocking. */
    for (unsigned mask = 0; mask < 8U; ++mask) {
        ds_assessment_init(&a);
        for (unsigned state = DS_UNKNOWN; state <= DS_TRUE; ++state)
            if (mask & (1U << state)) CHECK(ds_assessment_observe(&a, DS_CHECK_MOUNTED, (ds_truth)state, "synthetic:subset", "Evidence subset.") == 0);
        CHECK(ds_assessment_evaluate(&a) == 0);
        CHECK(a.status == (mask & (1U << DS_TRUE) ? DS_INSPECTION_BLOCKED : DS_INSPECTION_REVIEW));
    }
}
/** @brief Exercise classified disks, namespaces, APFS roles and common Linux storage layers. */
static void role_fixtures(void)
{
    const char *roles[] = {"disk", "namespace", "partition", "volume", "snapshot", "virtual", "composite", "unknown"};
    for (size_t i = 0; i < sizeof(roles) / sizeof(roles[0]); ++i) {
        ds_assessment a; ds_assessment_init(&a);
        ds_device d; ds_device_init(&d);
        d.role = roles[i];
        CHECK(ds_assessment_seed(&a, &d) == 0);
        CHECK(ds_assessment_evaluate(&a) == 0);
        CHECK(a.status == (i >= 2U && i <= 6U ? DS_INSPECTION_BLOCKED : DS_INSPECTION_REVIEW));
        /* Model names never create capability evidence, even for NVMe or USB. */
        CHECK(a.capability_count == 0);
        ds_device_free(&d);
    }
    const char *layers[] = {"loop", "dm", "md", "multipath", "APFS", "Fusion"};
    for (size_t i = 0; i < sizeof(layers) / sizeof(layers[0]); ++i) {
        ds_assessment a; ds_assessment_init(&a);
        ds_device d; ds_device_init(&d);
        d.role = i == 0U ? "virtual" : "composite";
        CHECK(ds_strings_add(&d.slaves, layers[i]) == 0);
        CHECK(ds_assessment_seed(&a, &d) == 0);
        CHECK(ds_assessment_evaluate(&a) == 0 && a.status == DS_INSPECTION_BLOCKED);
        CHECK(find(&a, "layered_backing", "slaves") != NULL);
        ds_device_free(&d);
    }
    ds_assessment a; ds_assessment_init(&a);
    ds_device d; ds_device_init(&d); d.transport = "usb"; d.role = "disk";
    CHECK(ds_assessment_seed(&a, &d) == 0 && ds_assessment_evaluate(&a) == 0);
    CHECK(find(&a, "bridge_scope_unverified", "usb_bridge") != NULL);
    CHECK(a.status == DS_INSPECTION_REVIEW && a.capability_count == 0);
    ds_device_free(&d);
}
/** @brief Separate method support from permission, unavailable APIs and contradictory flags. */
static void capability_fixtures(void)
{
    for (unsigned state = DS_CAP_UNKNOWN; state <= DS_CAP_UNAVAILABLE; ++state) {
        ds_assessment a; ds_assessment_init(&a);
        CHECK(ds_assessment_capability(&a, DS_METHOD_NVME_BLOCK, DS_CAP_SUPPORT, (ds_cap_state)state, "synthetic:identify", "Controlled fixture flag.") == 0);
        CHECK(ds_assessment_evaluate(&a) == 0 && a.status == DS_INSPECTION_REVIEW);
        CHECK(a.capabilities[0].state == (ds_cap_state)state);
    }
    for (unsigned dimension = DS_CAP_SUPPORT; dimension < DS_CAP_DIMENSION_COUNT; ++dimension) {
        ds_assessment a; complete(&a);
        CHECK(ds_assessment_capability(&a, DS_METHOD_NVME_BLOCK, (ds_cap_dimension)dimension, DS_CAP_UNSUPPORTED, "synthetic:contradiction", "Opposing flag, applicability or permission.") == 0);
        CHECK(ds_assessment_evaluate(&a) == 0 && a.status == DS_INSPECTION_REVIEW);
        CHECK(find(&a, "capability_conflict", ds_method_name(DS_METHOD_NVME_BLOCK)) != NULL);
        CHECK(a.capability_count == (size_t)DS_METHOD_COUNT * DS_CAP_DIMENSION_COUNT + 1U);
    }
    ds_assessment a; complete(&a);
    CHECK(ds_assessment_capability(&a, DS_METHOD_ATA_SECURITY, DS_CAP_ACCESS, DS_CAP_UNAVAILABLE, "synthetic:permission_denied", "No elevated retry permitted.") == 0);
    CHECK(ds_assessment_evaluate(&a) == 0 && a.status == DS_INSPECTION_REVIEW);
    CHECK(find(&a, "capability_unavailable", ds_method_name(DS_METHOD_ATA_SECURITY)) != NULL);
    /* Support plus denied access must require review even without conflicting flags. */
    complete(&a);
    for (size_t i = 0; i < a.capability_count; ++i) if (a.capabilities[i].dimension == DS_CAP_ACCESS) a.capabilities[i].state = DS_CAP_UNSUPPORTED;
    CHECK(ds_assessment_evaluate(&a) == 0 && a.status == DS_INSPECTION_REVIEW);
}
/** @brief Verify evidence ownership, exact limits, duplicate handling and stable order. */
static void bounds_and_order(void)
{
    ds_assessment a, b; complete(&a); ds_assessment_init(&b);
    for (size_t i = a.check_count; i; --i) {
        const ds_check_evidence *e = &a.checks[i - 1U];
        CHECK(ds_assessment_observe(&b, e->check, e->state, e->source, e->detail) == 0);
        CHECK(ds_assessment_observe(&b, e->check, e->state, e->source, e->detail) == 0);
    }
    for (size_t i = a.capability_count; i; --i) {
        const ds_cap_evidence *e = &a.capabilities[i - 1U];
        CHECK(ds_assessment_capability(&b, e->method, e->dimension, e->state, e->source, e->detail) == 0);
    }
    CHECK(ds_assessment_evaluate(&a) == 0 && ds_assessment_evaluate(&b) == 0);
    CHECK(a.status == DS_INSPECTION_NO_BLOCKERS && b.status == a.status);
    CHECK(a.finding_count == 1U && find(&a, "snapshot_not_authorization", "snapshot"));
    CHECK(a.check_count == b.check_count && a.capability_count == b.capability_count);
    CHECK(!memcmp(a.checks, b.checks, sizeof(a.checks)));
    CHECK(!memcmp(a.capabilities, b.capabilities, sizeof(a.capabilities)));
    CHECK(ds_assessment_evaluate(&a) == 0 && a.finding_count == 1U);
    ds_assessment_init(&a);
    char source[DS_ASSESS_SOURCE], detail[DS_ASSESS_DETAIL];
    memset(source, 'S', sizeof(source)); source[sizeof(source) - 1U] = '\0';
    memset(detail, 'D', sizeof(detail)); detail[sizeof(detail) - 1U] = '\0';
    CHECK(ds_assessment_observe(&a, DS_CHECK_USAGE, DS_UNKNOWN, source, detail) == 0);
    source[0] = 'X'; detail[0] = 'Y';
    CHECK(a.checks[0].source[0] == 'S' && a.checks[0].detail[0] == 'D');
    for (size_t i = 1; i < DS_ASSESS_LIMIT; ++i) {
        (void)snprintf(detail, sizeof(detail), "fixture-%zu", i);
        CHECK(ds_assessment_observe(&a, DS_CHECK_USAGE, DS_UNKNOWN, "test", detail) == 0);
    }
    CHECK(a.check_count == DS_ASSESS_LIMIT);
    CHECK(ds_assessment_observe(&a, DS_CHECK_USAGE, DS_UNKNOWN, "test", detail) == 0);
    CHECK(ds_assessment_observe(&a, DS_CHECK_USAGE, DS_UNKNOWN, "test", "overflow") == -1);
    CHECK(a.check_count == DS_ASSESS_LIMIT && ds_assessment_evaluate(&a) == -1);
    ds_assessment_init(&a);
    for (size_t i = 0; i < DS_ASSESS_LIMIT; ++i) {
        (void)snprintf(detail, sizeof(detail), "fixture-%zu", i);
        CHECK(ds_assessment_capability(&a, DS_METHOD_NVME_BLOCK, DS_CAP_SUPPORT, DS_CAP_UNKNOWN, "test", detail) == 0);
    }
    CHECK(ds_assessment_capability(&a, DS_METHOD_NVME_BLOCK, DS_CAP_SUPPORT, DS_CAP_UNKNOWN, "test", detail) == 0);
    CHECK(ds_assessment_capability(&a, DS_METHOD_NVME_BLOCK, DS_CAP_SUPPORT, DS_CAP_UNKNOWN, "test", "overflow") == -1);
    CHECK(ds_assessment_evaluate(&a) == -1 && a.status == DS_INSPECTION_NOT_EVALUATED);
    CHECK(!strcmp(ds_check_name((ds_check)-1), "invalid"));
    CHECK(!strcmp(ds_method_name(DS_METHOD_COUNT), "invalid"));
}
/** @brief Run all pure-policy fixtures without device nodes, privileges or data reads. */
int main(void)
{
    unknown_and_errors(); hazards_and_conflicts(); role_fixtures(); capability_fixtures(); bounds_and_order();
    printf("Assessment policy: %zu assertions passed.\n", checks);
    return 0;
}
