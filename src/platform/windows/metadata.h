/** @file
 * @brief Validate Windows wire metadata without depending on Windows handles or headers.
 */
#ifndef DISKSAN_WINDOWS_METADATA_H
#define DISKSAN_WINDOWS_METADATA_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/** @brief Classify bounded UTF-16 input before calling Windows conversion APIs. */
enum ds_wide_status { DS_WIDE_OK, DS_WIDE_INVALID, DS_WIDE_TOO_LONG };
/** @brief Check a bounded UTF-16 buffer, including its terminator and UTF-8 output limit. */
enum ds_wide_status ds_windows_wide_status(const void *bytes, size_t units);
/** @brief Validate every UTF-16 string and the final empty entry in a bounded native MULTI_SZ. */
bool ds_windows_multisz_valid(const void *bytes, size_t units);
/** @brief Check descriptor allocation, returned length, and embedded size consistently. */
bool ds_windows_descriptor_valid(size_t allocated, size_t returned, size_t declared, size_t minimum);
/** @brief Validate a string offset beyond a descriptor header and find its bounded terminator. */
bool ds_windows_string_valid(const void *bytes, size_t length, size_t minimum, size_t offset);
#endif
