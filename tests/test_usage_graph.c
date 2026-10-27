/**
 * @file
 * @brief Verify observed dependency components and identity changes using synthetic records.
 */
#include "disksan/usage.h"
#include <stdlib.h>
#include <string.h>

static size_t checks;
/** @brief Count assertions and emit no host inventory metadata. */
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)
/** @brief Append one owned synthetic node with a stable test-only native identity. */
static void add(ds_inventory *inventory, unsigned int number, const char *parent)
{
    ds_device d; ds_device_init(&d);
    char path[80]; (void)snprintf(path, sizeof(path), "/synthetic/node%u", number);
    d.platform = "linux"; d.role = parent ? "partition" : "disk";
    d.has_devnum = true; d.major_number = 8; d.minor_number = number;
    CHECK(ds_text_offer(&d, &d.path, path, "fixture") == 0);
    CHECK(ds_text_offer(&d, &d.sysfs_path, path, "fixture") == 0);
    CHECK(ds_text_offer(&d, &d.device_id, path, "sysfs:syspath+diskseq") == 0);
    CHECK(ds_text_offer(&d, &d.parent, parent, "fixture") == 0);
    CHECK(ds_inventory_add(inventory, &d) == 0);
    ds_device_free(&d);
}
/** @brief Build a disk/partition/mapper/holder component and one unrelated disk. */
static void graph_fixture(ds_inventory *inventory)
{
    add(inventory, 0, NULL); add(inventory, 1, "/synthetic/node0");
    add(inventory, 2, "/synthetic/node0"); add(inventory, 3, NULL); add(inventory, 4, NULL);
    inventory->items[3].role = "composite";
    CHECK(ds_strings_add(&inventory->items[3].slaves, "/synthetic/node1") == 0);
    CHECK(ds_strings_add(&inventory->items[1].holders, "/synthetic/node3") == 0);
}
/** @brief Propagate root/swap identifiers through parents and layers, never across absent edges. */
static void membership(void)
{
    ds_inventory inventory = {0}; graph_fixture(&inventory);
    for (size_t selected = 0; selected < 4U; ++selected) {
        ds_usage_graph graph;
        CHECK(ds_usage_graph_build(&inventory, &inventory.items[selected], &graph) == 0);
        CHECK(graph.links_resolved && graph.selected == selected);
        for (size_t i = 0; i < 5U; ++i) CHECK(graph.related[i] == (i < 4U));
        CHECK(ds_usage_graph_devnum(&inventory, &graph, 8, 3));
        CHECK(!ds_usage_graph_devnum(&inventory, &graph, 8, 4));
        CHECK(!ds_usage_graph_devnum(&inventory, &graph, 0, 0));
    }
    ds_usage_graph graph;
    CHECK(ds_usage_graph_build(&inventory, &inventory.items[4], &graph) == 0);
    CHECK(!graph.related[0] && graph.related[4]);
    CHECK(ds_strings_add(&inventory.items[0].holders, "/synthetic/unavailable") == 0);
    CHECK(ds_usage_graph_build(&inventory, &inventory.items[0], &graph) == 0 && !graph.links_resolved);
    CHECK(graph.related[3]);
    ds_inventory_free(&inventory);
}
/** @brief Reject duplicate/reused identities and explicit limits without exposing partial membership. */
static void ambiguity_and_limits(void)
{
    ds_inventory inventory = {0}; add(&inventory, 0, NULL); add(&inventory, 1, NULL);
    ds_usage_graph graph;
    inventory.items[1].minor_number = 0;
    CHECK(ds_usage_graph_build(&inventory, &inventory.items[0], &graph) == 3 && graph.count == 0);
    inventory.items[1].minor_number = 1;
    ds_device missing; ds_device_init(&missing);
    CHECK(ds_usage_graph_build(&inventory, &missing, &graph) == 3);
    size_t count = inventory.count;
    inventory.count = DS_USAGE_GRAPH_LIMIT + 1U;
    CHECK(ds_usage_graph_build(&inventory, &missing, &graph) == 1 && graph.count == 0);
    inventory.count = count;
    ds_inventory_free(&inventory); ds_device_free(&missing);
}
/** @brief Detect new/removed related nodes and changed generation while ignoring enumeration order. */
static void changes(void)
{
    ds_inventory before = {0}, after = {0}; graph_fixture(&before); graph_fixture(&after);
    ds_usage_graph a, b;
    CHECK(ds_usage_graph_build(&before, &before.items[0], &a) == 0);
    CHECK(ds_usage_graph_build(&after, &after.items[0], &b) == 0);
    CHECK(ds_usage_graph_same(&before, &a, &after, &b));
    ds_device swap = after.items[0]; after.items[0] = after.items[4]; after.items[4] = swap;
    CHECK(ds_usage_graph_build(&after, &before.items[0], &b) == 0);
    CHECK(ds_usage_graph_same(&before, &a, &after, &b));
    add(&after, 5, "/synthetic/node0");
    CHECK(ds_usage_graph_build(&after, &before.items[0], &b) == 0);
    CHECK(!ds_usage_graph_same(&before, &a, &after, &b));
    ds_inventory_free(&after); graph_fixture(&after);
    free(after.items[3].device_id.value); after.items[3].device_id.value = NULL;
    CHECK(ds_text_offer(&after.items[3], &after.items[3].device_id, "changed-generation", "fixture") == 0);
    CHECK(ds_usage_graph_build(&after, &before.items[0], &b) == 0);
    CHECK(!ds_usage_graph_same(&before, &a, &after, &b));
    ds_inventory_free(&before); ds_inventory_free(&after);
}
/** @brief Run all dependency fixtures without touching native devices or mount state. */
int main(void)
{
    membership(); ambiguity_and_limits(); changes();
    printf("Usage graph: %zu assertions passed.\n", checks);
    return 0;
}
