/** @file
 * @brief Inject counted allocation failures only into the separately linked synthetic plan CLI core.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

static size_t budget = SIZE_MAX, attempts, live;
static bool initialized;
/** @brief Report synthetic allocation counts and abort if command cleanup retained any tracked block. */
static void report(void)
{
    fprintf(stderr, "Fixture allocations: attempts=%zu live=%zu\n", attempts, live);
    if (live) abort();
}
/** @brief Read a test-only failure point once; production binaries do not link this module. */
static bool permitted(void)
{
    if (!initialized) {
        initialized = true;
        const char *text = getenv("DISKSAN_TEST_ALLOC_POINT");
        if (text) {
            char *end = NULL;
            unsigned long value = strtoul(text, &end, 10);
            if (!*text || *end || value > 1000UL) abort();
            budget = (size_t)value;
        }
        if (atexit(report)) abort();
    }
    ++attempts;
    if (budget == SIZE_MAX) return true;
    if (!budget) return false;
    --budget; return true;
}
/** @brief Fail a selected core allocation while retaining an exact live-block counter. */
void *ds_test_malloc(size_t size)
{
    if (!permitted()) return NULL;
    void *p = malloc(size);
    if (p) ++live;
    return p;
}
/** @brief Preserve the original block on allocation failure and count only newly created blocks. */
void *ds_test_realloc(void *old, size_t size)
{
    if (!permitted()) return NULL;
    bool fresh = old == NULL;
    void *p = realloc(old, size);
    if (p && fresh) ++live;
    return p;
}
/** @brief Count fixture releases without instrumenting the platform C runtime or production allocator. */
void ds_test_free(void *p)
{
    if (p) { if (!live) abort(); --live; }
    free(p);
}
