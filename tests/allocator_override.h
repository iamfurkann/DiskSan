/** @file
 * @brief Override allocation only after CRT declarations so native Windows imports stay intact.
 */
#ifndef DISKSAN_ALLOCATOR_OVERRIDE_H
#define DISKSAN_ALLOCATOR_OVERRIDE_H
#include <stdlib.h>
/** @brief Allocate through the deterministic core fault-injection counter. */
void *ds_test_malloc(size_t size);
/** @brief Reallocate through the deterministic core fault-injection counter. */
void *ds_test_realloc(void *old, size_t size);
/** @brief Release and count an instrumented core allocation. */
void ds_test_free(void *p);
/** @brief Route core malloc calls to the test implementation. */
#define malloc ds_test_malloc
/** @brief Route core realloc calls to the test implementation. */
#define realloc ds_test_realloc
/** @brief Route core free calls to the test implementation. */
#define free ds_test_free
#endif
