/** @file
 * @brief Exercise Windows descriptor and Unicode error fixtures on every native build host.
 */
#include "../src/platform/windows/metadata.h"
#include "disksan/device.h"
#include <stdlib.h>
#include <string.h>
static unsigned int checks;
/** @brief Count boundary assertions and stop immediately on a regression. */
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)
/** @brief Run synthetic buffer fixtures without opening Windows or Unix devices. */
int main(void)
{
    CHECK(ds_windows_descriptor_valid(100, 80, 80, 40));
    CHECK(!ds_windows_descriptor_valid(100, 39, 80, 40));
    CHECK(!ds_windows_descriptor_valid(100, 101, 80, 40));
    CHECK(!ds_windows_descriptor_valid(100, 80, 81, 40));
    CHECK(!ds_windows_descriptor_valid(100, 80, 39, 40));
    CHECK(!ds_windows_descriptor_valid(SIZE_MAX, 80, 80, 40));
    char buffer[80]; memset(buffer, 'X', sizeof(buffer)); buffer[79] = '\0';
    CHECK(ds_windows_string_valid(buffer, sizeof(buffer), 40, 0));
    CHECK(ds_windows_string_valid(buffer, sizeof(buffer), 40, 40));
    CHECK(ds_windows_string_valid(buffer, sizeof(buffer), 40, 79));
    CHECK(!ds_windows_string_valid(buffer, sizeof(buffer), 40, 39));
    CHECK(!ds_windows_string_valid(buffer, sizeof(buffer), 40, 80));
    CHECK(!ds_windows_string_valid(buffer, sizeof(buffer), 40, SIZE_MAX));
    buffer[79] = 'X'; CHECK(!ds_windows_string_valid(buffer, sizeof(buffer), 40, 40));
    const uint16_t valid[] = {'A', 0xd83d, 0xde00, 0};
    const uint16_t high[] = {0xd800, 0}, low[] = {0xdc00, 0}, wrong[] = {0xd800, 'A', 0}, empty[] = {0};
    CHECK(ds_windows_wide_status(valid, 4) == DS_WIDE_OK);
    CHECK(ds_windows_wide_status(valid, 3) == DS_WIDE_INVALID);
    CHECK(ds_windows_wide_status(high, 2) == DS_WIDE_INVALID);
    CHECK(ds_windows_wide_status(high, 1) == DS_WIDE_INVALID);
    CHECK(ds_windows_wide_status(low, 2) == DS_WIDE_INVALID);
    CHECK(ds_windows_wide_status(wrong, 3) == DS_WIDE_INVALID);
    CHECK(ds_windows_wide_status(empty, 1) == DS_WIDE_INVALID);
    uint16_t long_path[DS_TEXT_LIMIT + 2U];
    for (size_t i = 0; i < DS_TEXT_LIMIT + 1U; ++i) long_path[i] = 'a';
    long_path[DS_TEXT_LIMIT + 1U] = 0;
    CHECK(ds_windows_wide_status(long_path, DS_TEXT_LIMIT + 2U) == DS_WIDE_TOO_LONG);
    long_path[DS_TEXT_LIMIT] = 0;
    CHECK(ds_windows_wide_status(long_path, DS_TEXT_LIMIT + 1U) == DS_WIDE_OK);
    printf("%u Windows metadata boundary assertions passed (synthetic, no native API calls).\n", checks);
    return 0;
}
