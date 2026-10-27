/**
 * @file
 * @brief Compare target snapshots without invoking native discovery or usage collectors.
 */
#include "disksan/target.h"
#include <string.h>

/** @brief Recognize meaningful metadata without treating unknown labels as identities. */
static bool known(const char *value)
{
    return value && *value && strcmp(value, "unknown") != 0;
}
/** @brief Compare identity text; loss of previously known identity fails closed. */
static bool changed_text(const char *before, const char *after, bool *partial)
{
    if (!known(before) || !known(after)) {
        *partial = true;
        return known(before) && !known(after);
    }
    return strcmp(before, after) != 0;
}
/** @brief Compare optional metadata only when observed and flag loss without inventing facts. */
static bool changed_optional(const char *before, const char *after, bool *partial)
{
    if (!before && !after) return false;
    return changed_text(before, after, partial);
}
/** @brief Detect changes or loss of known capacity, geometry and read-only state. */
static bool changed_number(const ds_number *before, const ds_number *after, bool *partial)
{
    if (before->known != after->known) { *partial = true; return before->known; }
    return before->known && before->value != after->value;
}
/** @brief Compare relationship sets independently of native enumeration order. */
static bool changed_links(const ds_strings *before, const ds_strings *after)
{
    if (before->count != after->count) return true;
    for (size_t i = 0; i < before->count; ++i) {
        bool found = false;
        for (size_t j = 0; j < after->count; ++j) if (!strcmp(before->items[i], after->items[j])) found = true;
        if (!found) return true;
    }
    return false;
}
/** @brief Check whether the existing native identity has an observed kernel generation scope. */
static bool generation_observed(const ds_device *d)
{
    if (!known(d->device_id.value) || !d->device_id.source || !d->platform) return false;
    return (!strcmp(d->platform, "linux") && !strcmp(d->device_id.source, "sysfs:syspath+diskseq")) ||
           (!strcmp(d->platform, "macos") && !strcmp(d->device_id.source, "iokit:registry_entry_id"));
}
/** @brief Compare native identity, role, relevant state and relationships conservatively. */
int ds_target_compare(const ds_device *before, const ds_device *after, ds_assessment *result, ds_target_status *status)
{
    bool partial = false;
    bool changed = changed_text(before->platform, after->platform, &partial);
    changed |= changed_text(before->device_id.value, after->device_id.value, &partial);
    changed |= changed_text(before->path.value, after->path.value, &partial);
    changed |= changed_text(before->role, after->role, &partial);
    changed |= before->has_devnum && (!after->has_devnum || before->major_number != after->major_number || before->minor_number != after->minor_number);
    partial |= before->has_devnum != after->has_devnum;
    changed |= changed_optional(before->sysfs_path.value, after->sysfs_path.value, &partial);
    changed |= changed_optional(before->parent.value, after->parent.value, &partial);
    changed |= changed_optional(before->controller.value, after->controller.value, &partial);
    changed |= changed_optional(before->serial.value, after->serial.value, &partial);
    changed |= changed_optional(before->wwn.value, after->wwn.value, &partial);
    changed |= changed_optional(before->eui.value, after->eui.value, &partial);
    changed |= changed_optional(before->nguid.value, after->nguid.value, &partial);
    changed |= changed_number(&before->size_bytes, &after->size_bytes, &partial);
    changed |= changed_number(&before->logical_sector_size, &after->logical_sector_size, &partial);
    changed |= changed_number(&before->physical_sector_size, &after->physical_sector_size, &partial);
    changed |= changed_number(&before->read_only, &after->read_only, &partial);
    changed |= changed_links(&before->holders, &after->holders) || changed_links(&before->slaves, &after->slaves);
    bool generation = generation_observed(before) && generation_observed(after);
    if ((before->platform && (!strcmp(before->platform, "linux") || !strcmp(before->platform, "macos"))) &&
        (!before->has_devnum || !after->has_devnum)) partial = true;
    partial |= !generation || before->warnings.count != 0U || after->warnings.count != 0U;
    *status = changed ? DS_TARGET_CHANGED : partial ? DS_TARGET_PARTIAL : DS_TARGET_MATCH;
    if (ds_assessment_observe(result, DS_CHECK_IDENTITY, changed ? DS_FALSE : partial ? DS_UNKNOWN : DS_TRUE,
                              "resolver:before_after", changed ? "Identity, role, topology or relevant state changed during inspection." :
                              partial ? "Observed fields match, but missing evidence or warnings limit identity verification." :
                              "Observed native identity and relevant state match at both ends of the window.")) return -1;
    return ds_assessment_observe(result, DS_CHECK_GENERATION, generation && !changed ? DS_TRUE : DS_UNKNOWN,
                                "resolver:native_generation", "Generation applies only to observed snapshots; ABA changes and future identity are not guaranteed.");
}
