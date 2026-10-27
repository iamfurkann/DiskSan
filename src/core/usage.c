/**
 * @file
 * @brief Connect existing native relationships and detect changes without querying devices.
 */
#include "disksan/usage.h"
#include "disksan/target.h"
#include <string.h>

/** @brief Compare present text without merging missing identities. */
static bool equal(const char *a, const char *b)
{
    return a && b && *a && !strcmp(a, b);
}
/** @brief Require both a platform and native identifier for a record match. */
static bool same_id(const ds_device *a, const ds_device *b)
{
    return equal(a->platform, b->platform) && equal(a->device_id.value, b->device_id.value);
}
/** @brief Find a union root with a strictly bounded iteration count. */
static size_t component(size_t *parents, size_t index)
{
    while (parents[index] != index) { parents[index] = parents[parents[index]]; index = parents[index]; }
    return index;
}
/** @brief Resolve one explicit native relationship, rejecting ambiguous aliases conservatively. */
static void link_record(const ds_inventory *inventory, size_t from, const char *path, size_t *parents, bool *resolved)
{
    if (!path || !*path) return;
    size_t match = SIZE_MAX;
    for (size_t i = 0; i < inventory->count; ++i) {
        const ds_device *d = &inventory->items[i];
        if (!equal(d->platform, inventory->items[from].platform)) continue;
        if (equal(path, d->path.value) || equal(path, d->sysfs_path.value) || equal(path, d->device_id.value)) {
            if (match != SIZE_MAX) { *resolved = false; return; }
            match = i;
        }
    }
    if (match == SIZE_MAX || match == from) { *resolved = false; return; }
    parents[component(parents, from)] = component(parents, match);
}
/** @brief Build a bounded overapproximation of observed dependencies including reverse consumers. */
int ds_usage_graph_build(const ds_inventory *inventory, const ds_device *selected, ds_usage_graph *graph)
{
    memset(graph, 0, sizeof(*graph));
    if (inventory->count > DS_USAGE_GRAPH_LIMIT) return 1;
    size_t index = SIZE_MAX, parents[DS_USAGE_GRAPH_LIMIT];
    for (size_t i = 0; i < inventory->count; ++i) {
        parents[i] = i;
        if (same_id(&inventory->items[i], selected)) {
            if (index != SIZE_MAX) return 3;
            index = i;
        }
        for (size_t j = 0; j < i; ++j) {
            const ds_device *a = &inventory->items[i], *b = &inventory->items[j];
            if (same_id(a, b) || (equal(a->platform, b->platform) && a->has_devnum && b->has_devnum &&
                                  a->major_number == b->major_number && a->minor_number == b->minor_number)) return 3;
        }
    }
    if (index == SIZE_MAX) return 3;
    bool resolved = inventory->warnings.count == 0U;
    for (size_t i = 0; i < inventory->count; ++i) {
        const ds_device *d = &inventory->items[i];
        if (d->holders.count > DS_USAGE_GRAPH_LIMIT || d->slaves.count > DS_USAGE_GRAPH_LIMIT) return 1;
        if (!d->device_id.value || !d->platform) resolved = false;
        for (size_t w = 0; w < d->warnings.count; ++w)
            if (strstr(d->warnings.items[w], "topology") || strstr(d->warnings.items[w], "parent")) resolved = false;
        if (d->role && !strcmp(d->role, "partition") && !d->parent.value) resolved = false;
        link_record(inventory, i, d->parent.value, parents, &resolved);
        for (size_t j = 0; j < d->holders.count; ++j) link_record(inventory, i, d->holders.items[j], parents, &resolved);
        for (size_t j = 0; j < d->slaves.count; ++j) link_record(inventory, i, d->slaves.items[j], parents, &resolved);
    }
    size_t root = component(parents, index);
    graph->count = inventory->count; graph->selected = index; graph->links_resolved = resolved;
    for (size_t i = 0; i < inventory->count; ++i) graph->related[i] = component(parents, i) == root;
    return 0;
}
/** @brief Match native identifiers without assuming a path name maps to a filesystem. */
bool ds_usage_graph_devnum(const ds_inventory *inventory, const ds_usage_graph *graph, unsigned int major_number, unsigned int minor_number)
{
    if (graph->count != inventory->count || graph->count > DS_USAGE_GRAPH_LIMIT) return false;
    for (size_t i = 0; i < graph->count; ++i) {
        const ds_device *d = &inventory->items[i];
        if (graph->related[i] && d->has_devnum && d->major_number == major_number && d->minor_number == minor_number) return true;
    }
    return false;
}
/** @brief Detect changed related identities, geometry or topology using the target comparator. */
bool ds_usage_graph_same(const ds_inventory *before, const ds_usage_graph *first, const ds_inventory *after, const ds_usage_graph *last)
{
    if (first->count != before->count || last->count != after->count ||
        first->count > DS_USAGE_GRAPH_LIMIT || last->count > DS_USAGE_GRAPH_LIMIT || first->links_resolved != last->links_resolved) return false;
    size_t matched = 0, wanted = 0;
    for (size_t i = 0; i < last->count; ++i) if (last->related[i]) ++wanted;
    ds_assessment comparison;
    for (size_t i = 0; i < first->count; ++i) if (first->related[i]) {
        bool found = false;
        for (size_t j = 0; j < last->count; ++j) if (last->related[j] && same_id(&before->items[i], &after->items[j])) {
            ds_target_status status;
            ds_assessment_init(&comparison);
            if (ds_target_compare(&before->items[i], &after->items[j], &comparison, &status) || status == DS_TARGET_CHANGED) return false;
            found = true; ++matched; break;
        }
        if (!found) return false;
    }
    return matched == wanted;
}
