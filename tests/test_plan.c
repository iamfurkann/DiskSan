/** @file
 * @brief Verify inspection-plan truth tables, owned bounds and failure poisoning using only synthetic observations.
 */
#include "disksan/plan.h"
#include <stdlib.h>
#include <string.h>

static size_t checks;
/** @brief Stop on a synthetic invariant failure without emitting device identities. */
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)

/** @brief Prepare complete synthetic observations without native access or operation authority. */
static void complete(ds_assessment *a)
{
    ds_assessment_init(a);
    for (unsigned c = 0; c < DS_CHECK_COUNT; ++c) {
        bool hazard = c >= DS_CHECK_MOUNTED && c <= DS_CHECK_USB_BRIDGE;
        CHECK(ds_assessment_observe(a, (ds_check)c, hazard ? DS_FALSE : DS_TRUE,
                                   "synthetic:complete", "Fixture-only observation.") == 0);
    }
    for (unsigned m = 0; m < DS_METHOD_COUNT; ++m) for (unsigned d = 0; d < DS_CAP_DIMENSION_COUNT; ++d)
        CHECK(ds_assessment_capability(a, (ds_method)m, (ds_cap_dimension)d, DS_CAP_SUPPORTED,
                                      "synthetic:capability", "Synthetic flags; no method was executed.") == 0);
    CHECK(ds_assessment_evaluate(a) == 0);
}
/** @brief Populate an initialized owned fake device without an OS selector. */
static void device(ds_device *d)
{
    ds_device_init(d); d->role = "disk"; d->platform = "linux"; d->transport = "nvme"; d->protocol = "nvme";
    CHECK(ds_text_offer(d, &d->path, "/synthetic/plan-device", "fixture") == 0);
    CHECK(ds_text_offer(d, &d->device_id, "synthetic:generation", "fixture") == 0);
    CHECK(ds_number_offer(d, &d->read_only, 0, "fixture") == 0);
}
/** @brief Every result leaves independent future prerequisites unimplemented, including complete fixtures. */
static void no_authority(const ds_plan *p)
{
    CHECK(!p->failed && p->status != DS_PLAN_NOT_EVALUATED);
    for (unsigned i = DS_PLAN_PRIVILEGE; i < DS_PLAN_REQUIREMENT_COUNT; ++i)
        CHECK(p->prerequisites[i] == DS_MODEL_UNIMPLEMENTED);
}
/** @brief Check all truth subsets on every hazard; positive observations cannot be canceled by unknown or false. */
static void truth_tables(const ds_device *d)
{
    for (unsigned c = DS_CHECK_MOUNTED; c <= DS_CHECK_LOGICAL; ++c) for (unsigned mask = 0; mask < 8U; ++mask) {
        ds_assessment a; complete(&a);
        /* Replace the selected check only, preserving all other complete observations. */
        a.checks[c] = a.checks[--a.check_count]; a.status = DS_INSPECTION_NOT_EVALUATED;
        for (unsigned state = DS_UNKNOWN; state <= DS_TRUE; ++state)
            if (mask & (1U << state)) CHECK(ds_assessment_observe(&a, (ds_check)c, (ds_truth)state,
                                          "synthetic:subset", "Truth-table input.") == 0);
        CHECK(ds_assessment_evaluate(&a) == 0);
        ds_plan p; CHECK(ds_plan_build(d, &a, &p) == 0); no_authority(&p);
        CHECK((p.status == DS_PLAN_BLOCKED) == ((mask & (1U << DS_TRUE)) != 0U));
        if ((mask & (1U << DS_TRUE)) && (mask & (1U << DS_FALSE))) CHECK(p.observations[c] == DS_UNKNOWN);
    }
    for (unsigned c = DS_CHECK_IDENTITY; c <= DS_CHECK_PHYSICAL_SCOPE; ++c) {
        ds_assessment a; complete(&a);
        CHECK(ds_assessment_observe(&a, (ds_check)c, DS_UNKNOWN, "synthetic:missing", "Incomplete observation.") == 0);
        CHECK(ds_assessment_evaluate(&a) == 0);
        ds_plan p; CHECK(ds_plan_build(d, &a, &p) == 0); no_authority(&p);
        CHECK(p.status == (c <= DS_CHECK_GENERATION ? DS_PLAN_INSUFFICIENT : DS_PLAN_REVIEW));
    }
    ds_assessment a; complete(&a);
    CHECK(ds_assessment_observe(&a, DS_CHECK_IDENTITY, DS_FALSE, "synthetic:changed", "Changed identity.") == 0);
    CHECK(ds_assessment_evaluate(&a) == 0);
    ds_plan p; CHECK(ds_plan_build(d, &a, &p) == 0 && p.status == DS_PLAN_BLOCKED);
    CHECK(p.prerequisites[DS_PLAN_IDENTITY] == DS_MODEL_BLOCK);
}
/** @brief Classified layers, holders and read-only flags cannot be hidden by optimistic supplied checks. */
static void snapshot_hazards(void)
{
    const char *roles[] = {"partition", "volume", "snapshot", "virtual", "composite"};
    ds_device d; device(&d); ds_assessment a; complete(&a); ds_plan p;
    for (size_t i = 0; i < sizeof(roles) / sizeof(roles[0]); ++i) {
        d.role = roles[i]; CHECK(ds_plan_build(&d, &a, &p) == 0 && p.status == DS_PLAN_BLOCKED); no_authority(&p);
    }
    d.role = "disk"; d.read_only.value = 1;
    CHECK(ds_plan_build(&d, &a, &p) == 0 && p.status == DS_PLAN_BLOCKED);
    d.read_only.value = 0;
    CHECK(ds_strings_add(&d.holders, "synthetic:holder") == 0);
    CHECK(ds_plan_build(&d, &a, &p) == 0 && p.status == DS_PLAN_BLOCKED);
    ds_strings_free(&d.holders);
    CHECK(ds_strings_add(&d.slaves, "synthetic:backing") == 0);
    CHECK(ds_plan_build(&d, &a, &p) == 0 && p.status == DS_PLAN_BLOCKED);
    ds_strings_free(&d.slaves); d.transport = "usb";
    CHECK(ds_plan_build(&d, &a, &p) == 0 && p.status == DS_PLAN_REVIEW);
    CHECK(p.prerequisites[DS_PLAN_TOPOLOGY] == DS_MODEL_REVIEW);
    d.transport = "nvme";
    char *saved = d.device_id.value; d.device_id.value = NULL;
    CHECK(ds_plan_build(&d, &a, &p) == 0 && p.status == DS_PLAN_INSUFFICIENT);
    d.device_id.value = saved; saved = d.path.value; d.path.value = NULL;
    CHECK(ds_plan_build(&d, &a, &p) == 0 && p.status == DS_PLAN_INSUFFICIENT && !p.target_observed);
    d.path.value = saved; ds_device_free(&d);
}
/** @brief Unknown, unsupported, unavailable and conflicting capability evidence remain separate copied facts. */
static void capabilities(const ds_device *d)
{
    for (unsigned method = 0; method < DS_METHOD_COUNT; ++method)
        for (unsigned dim = 0; dim < DS_CAP_DIMENSION_COUNT; ++dim)
            for (unsigned state = DS_CAP_UNKNOWN; state <= DS_CAP_UNAVAILABLE; ++state) {
                ds_assessment a; complete(&a);
                CHECK(ds_assessment_capability(&a, (ds_method)method, (ds_cap_dimension)dim, (ds_cap_state)state,
                                              "synthetic:other", "Independent alternate evidence.") == 0);
                CHECK(ds_assessment_evaluate(&a) == 0);
                ds_plan p; CHECK(ds_plan_build(d, &a, &p) == 0); no_authority(&p);
                CHECK(p.prerequisites[DS_PLAN_METHOD_EVIDENCE] == (state == DS_CAP_SUPPORTED ? DS_MODEL_OBSERVED : DS_MODEL_REVIEW));
                CHECK(p.evidence.capability_count == a.capability_count);
                for (size_t i = 0; i < a.capability_count; ++i) {
                    const ds_cap_evidence *actual = &p.evidence.capabilities[i], *original = &a.capabilities[i];
                    CHECK(actual->method == original->method && actual->dimension == original->dimension && actual->state == original->state);
                    CHECK(!strcmp(actual->source, original->source) && !strcmp(actual->detail, original->detail));
                }
            }
}
/** @brief Invalid counts, states, strings and stale evaluations fail before a partial plan can be retained. */
static void invalid_inputs(const ds_device *d)
{
    for (unsigned fault = 0; fault < 18U; ++fault) {
        ds_assessment a; complete(&a); ds_plan p;
        switch (fault) {
            case 0: a.failed = true; break;
            case 1: a.status = DS_INSPECTION_NOT_EVALUATED; break;
            case 2: a.status = (ds_inspection_status)99; break;
            case 3: a.check_count = DS_ASSESS_LIMIT + 1U; break;
            case 4: a.capability_count = SIZE_MAX; break;
            case 5: a.finding_count = SIZE_MAX; break;
            case 6: a.checks[0].check = DS_CHECK_COUNT; break;
            case 7: a.checks[0].state = (ds_truth)-1; break;
            case 8: memset(a.checks[0].source, 'X', sizeof(a.checks[0].source)); break;
            case 9: memset(a.checks[0].detail, 'X', sizeof(a.checks[0].detail)); break;
            case 10: a.capabilities[0].method = DS_METHOD_COUNT; break;
            case 11: a.capabilities[0].dimension = DS_CAP_DIMENSION_COUNT; break;
            case 12: a.capabilities[0].state = (ds_cap_state)99; break;
            case 13: a.capabilities[0].source[0] = '\0'; break;
            case 14: memset(a.capabilities[0].detail, 'X', sizeof(a.capabilities[0].detail)); break;
            case 15: a.probe.status = (ds_probe_status)99; break;
            case 16: a.status = DS_INSPECTION_BLOCKED; break;
            default: a.checks[DS_CHECK_SYSTEM].state = DS_TRUE; break;
        }
        CHECK(ds_plan_build(d, &a, &p) == -1);
        CHECK(p.failed && p.status == DS_PLAN_NOT_EVALUATED && !p.evidence.check_count && !p.evidence.capability_count);
    }
    ds_assessment a; complete(&a); ds_plan p;
    CHECK(ds_plan_build(NULL, &a, &p) == -1 && p.failed);
    CHECK(ds_plan_build(d, NULL, &p) == -1 && p.failed);
    CHECK(ds_plan_build(d, &p.evidence, &p) == -1 && p.failed);
    ds_device malformed = *d;
    char too_long[DS_TEXT_LIMIT + 1U]; memset(too_long, 'X', sizeof(too_long));
    malformed.path.value = too_long;
    CHECK(ds_plan_build(&malformed, &a, &p) == -1 && p.failed);
    malformed = *d; malformed.role = too_long;
    CHECK(ds_plan_build(&malformed, &a, &p) == -1 && p.failed);
    complete(&a);
    for (unsigned i = (unsigned)a.check_count; i < DS_ASSESS_LIMIT; ++i) {
        char label[40]; CHECK(snprintf(label, sizeof(label), "synthetic:extra-%u", i) > 0);
        CHECK(ds_assessment_observe(&a, DS_CHECK_USAGE, DS_TRUE, label, "Bounded evidence.") == 0);
    }
    CHECK(ds_assessment_evaluate(&a) == 0);
    CHECK(ds_plan_build(d, &a, &p) == -1 && p.failed && !p.evidence.check_count);
}
/** @brief Rebuilding ignores untrusted derived pointers and owns copies that survive input mutation. */
static void ownership_and_labels(const ds_device *d)
{
    ds_assessment a; complete(&a); ds_plan first, second;
    CHECK(ds_plan_build(d, &a, &first) == 0 && first.status == DS_PLAN_INSPECTION_ONLY); no_authority(&first);
    for (size_t i = 0; i < a.finding_count; ++i) a.findings[i].code = NULL;
    CHECK(ds_plan_build(d, &a, &second) == 0 && second.status == first.status);
    CHECK(second.evidence.findings[0].code != NULL);
    a.checks[0].source[0] = 'X'; a.capabilities[0].source[0] = 'X';
    CHECK(second.evidence.checks[0].source[0] != 'X' && second.evidence.capabilities[0].source[0] != 'X');
    for (unsigned i = 0; i < DS_PLAN_REQUIREMENT_COUNT; ++i) {
        CHECK(strcmp(ds_plan_requirement_code((ds_plan_requirement)i), "invalid") != 0);
        CHECK(strlen(ds_plan_requirement_detail((ds_plan_requirement)i)) < DS_ASSESS_DETAIL);
        CHECK(strlen(ds_plan_requirement_scope((ds_plan_requirement)i)) < DS_ASSESS_SOURCE);
    }
    CHECK(!strcmp(ds_plan_state_name((ds_plan_state)-1), "invalid"));
    CHECK(!strcmp(ds_model_state_name((ds_model_state)99), "invalid"));
    CHECK(!strcmp(ds_plan_requirement_code(DS_PLAN_REQUIREMENT_COUNT), "invalid"));
    CHECK(!strcmp(ds_plan_requirement_detail((ds_plan_requirement)-1), "invalid"));
    CHECK(!strcmp(ds_plan_requirement_scope((ds_plan_requirement)99), "invalid"));
    ds_plan_init(&first); ds_plan_init(&first); CHECK(first.status == DS_PLAN_NOT_EVALUATED && !first.failed);
}
/** @brief Reject corrupted derived results before output and rebuild untrusted finding labels from owned evidence. */
static void output_validation(const ds_device *d)
{
    ds_assessment a; complete(&a);
    for (unsigned json = 0; json < 2U; ++json) for (unsigned scenario = 0; scenario < 13U; ++scenario) {
        ds_plan p; CHECK(ds_plan_build(d, &a, &p) == 0);
        ds_plan_origin origin = DS_PLAN_SYNTHETIC_OBSERVATIONS;
        const ds_plan *input = &p;
        const ds_device *snapshot = d;
        switch (scenario) {
            case 0: p.failed = true; break;
            case 1: p.status = DS_PLAN_NOT_EVALUATED; break;
            case 2: p.target_observed = false; break;
            case 3: p.observations[DS_CHECK_IDENTITY] = DS_UNKNOWN; break;
            case 4: p.observation_masks[DS_CHECK_MOUNTED] = 0; break;
            case 5: p.prerequisites[DS_PLAN_EXECUTION] = DS_MODEL_OBSERVED; break;
            case 6: p.evidence.check_count = SIZE_MAX; break;
            case 7: p.evidence.capabilities[0].dimension = (ds_cap_dimension)99; break;
            case 8: memset(p.evidence.checks[0].source, 'x', sizeof(p.evidence.checks[0].source)); break;
            case 9: origin = (ds_plan_origin)-1; break;
            case 10: input = NULL; break;
            case 11: snapshot = NULL; break;
            default: p.evidence.status = DS_INSPECTION_NOT_EVALUATED; break;
        }
        FILE *out = tmpfile(); CHECK(out != NULL);
        CHECK(ds_plan_output(out, snapshot, input, json != 0U, origin) == 1);
        CHECK(ds_simulation_output(out, snapshot, input, json != 0U, origin) == 1);
        CHECK(ftell(out) == 0L); CHECK(fclose(out) == 0);
    }
    ds_plan p; CHECK(ds_plan_build(d, &a, &p) == 0);
    for (size_t i = 0; i < p.evidence.finding_count; ++i) {
        p.evidence.findings[i].code = NULL; p.evidence.findings[i].message = NULL;
    }
    for (unsigned json = 0; json < 2U; ++json) {
        FILE *out = tmpfile(); CHECK(out != NULL);
        CHECK(ds_plan_output(out, d, &p, json != 0U, DS_PLAN_SYNTHETIC_OBSERVATIONS) == 0);
        CHECK(ds_simulation_output(out, d, &p, json != 0U, DS_PLAN_SYNTHETIC_OBSERVATIONS) == 0);
        CHECK(ftell(out) > 0L); CHECK(fclose(out) == 0);
    }
}
/** @brief Run the synthetic inspection-only policy contract without linking any native adapter. */
int main(void)
{
    ds_device d; device(&d);
    truth_tables(&d); snapshot_hazards(); capabilities(&d); invalid_inputs(&d); ownership_and_labels(&d); output_validation(&d);
    ds_device_free(&d);
    printf("Inspection-only plan: %zu assertions passed.\n", checks);
    return 0;
}
