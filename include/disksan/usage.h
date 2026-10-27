/**
 * @file
 * @brief Resolve bounded observed storage relationships without asserting physical completeness.
 */
#ifndef DISKSAN_USAGE_H
#define DISKSAN_USAGE_H
#include "disksan/device.h"
/** @brief Maximum records accepted by a usage graph; overflow is explicit failure. */
#define DS_USAGE_GRAPH_LIMIT 1024U
/** @brief Own membership of the selected record's observed connected component. */
typedef struct {
    bool related[DS_USAGE_GRAPH_LIMIT]; /**< @brief True for related inventory indices. */
    size_t count; /**< @brief Inventory length validated against the fixed bound. */
    size_t selected; /**< @brief Unique selected record index. */
    bool links_resolved; /**< @brief Every explicit link resolved; not full physical topology. */
} ds_usage_graph;
/**
 * @brief Connect observed parent/holder/slave links conservatively in both directions.
 * @param[in] inventory Borrowed native snapshot; may contain partial records.
 * @param[in] selected Borrowed selected identity to locate uniquely.
 * @param[out] graph Receives bounded membership; cleared on failure.
 * @return 0 on success, 3 if selection or native identity is ambiguous/missing, 1 on limits.
 * @note Shared parent links can include siblings; this intentionally overapproximates dependencies.
 */
int ds_usage_graph_build(const ds_inventory *inventory, const ds_device *selected, ds_usage_graph *graph);
/**
 * @brief Match a filesystem or block identifier against observed component members.
 * @param[in] inventory Borrowed snapshot used to build graph.
 * @param[in] graph Valid graph for inventory.
 * @param[in] major_number Valid native major identifier.
 * @param[in] minor_number Valid native minor identifier.
 * @return True on an observed match, not a claim about unmatched or hidden consumers.
 */
bool ds_usage_graph_devnum(const ds_inventory *inventory, const ds_usage_graph *graph, unsigned int major_number, unsigned int minor_number);
/**
 * @brief Compare related records across observation windows without allocating evidence.
 * @param[in] before Borrowed earlier inventory.
 * @param[in] first Graph built from before.
 * @param[in] after Borrowed later inventory.
 * @param[in] last Graph built from after.
 * @return True if membership and relevant identities/relationships match; false if changed.
 * @note Equality is snapshot evidence, not a lock or authorization.
 */
bool ds_usage_graph_same(const ds_inventory *before, const ds_usage_graph *first, const ds_inventory *after, const ds_usage_graph *last);
#endif
