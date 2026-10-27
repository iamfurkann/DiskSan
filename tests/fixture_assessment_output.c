/**
 * @file
 * @brief Render synthetic method evidence including contradictions and poisoned results.
 */
#include "disksan/assessment.h"
#include <string.h>

/** @brief Build fixture-only capability states and never query a native device. */
int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "unknown";
    ds_device device; ds_device_init(&device);
    ds_assessment a; ds_assessment_init(&a);
    ds_cap_state state = !strcmp(mode, "supported") || !strcmp(mode, "conflict") ? DS_CAP_SUPPORTED :
                         !strcmp(mode, "unsupported") ? DS_CAP_UNSUPPORTED : !strcmp(mode, "unavailable") ? DS_CAP_UNAVAILABLE : DS_CAP_UNKNOWN;
    int rc = ds_assessment_capability(&a, DS_METHOD_NVME_BLOCK, DS_CAP_SUPPORT, state, "fixture:identify", "Synthetic flags only.");
    if (!rc && !strcmp(mode, "conflict")) rc = ds_assessment_capability(&a, DS_METHOD_NVME_BLOCK, DS_CAP_SUPPORT, DS_CAP_UNSUPPORTED, "fixture:contradiction", "Contradictory synthetic flags.");
    if (!rc && strcmp(mode, "unevaluated")) rc = ds_assessment_evaluate(&a);
    if (!strcmp(mode, "failed")) a.failed = true;
    if (!rc) rc = ds_assessment_output(stdout, &device, &a, true, true);
    ds_device_free(&device);
    return rc ? 1 : 0;
}
