/** @file
 * @brief Bound Windows descriptor strings and distinguish malformed from oversized UTF-16.
 */
#include "metadata.h"
#include "disksan/device.h"
#include <string.h>

/** @brief Read one UTF-16 code unit without relying on alignment or wchar_t width. */
static uint16_t unit_at(const void *bytes, size_t index)
{
    uint16_t value;
    memcpy(&value, (const unsigned char *)bytes + index * sizeof(value), sizeof(value));
    return value;
}
/** @brief Validate surrogates and a terminator before any unbounded Windows API string read. */
enum ds_wide_status ds_windows_wide_status(const void *bytes, size_t units)
{
    size_t utf8_size = 0;
    for (size_t i = 0; i < units; ++i) {
        uint16_t value = unit_at(bytes, i);
        if (!value) return utf8_size ? DS_WIDE_OK : DS_WIDE_INVALID;
        if (value >= 0xd800U && value <= 0xdbffU) {
            if (++i >= units) return DS_WIDE_INVALID;
            uint16_t low = unit_at(bytes, i);
            if (low < 0xdc00U || low > 0xdfffU) return DS_WIDE_INVALID;
            utf8_size += 4U;
        } else if (value >= 0xdc00U && value <= 0xdfffU) return DS_WIDE_INVALID;
        else utf8_size += value < 0x80U ? 1U : value < 0x800U ? 2U : 3U;
        if (utf8_size > DS_TEXT_LIMIT) return DS_WIDE_TOO_LONG;
    }
    return DS_WIDE_INVALID;
}
/** @brief Reject malformed UTF-16, missing terminators, embedded empty entries and trailing garbage. */
bool ds_windows_multisz_valid(const void *bytes, size_t units)
{
    if (!bytes || !units || units > DS_TEXT_LIMIT) return false;
    size_t start = 0;
    while (start < units) {
        if (!unit_at(bytes, start)) return start + 1U == units ||
            (start == 0U && units == 2U && !unit_at(bytes, 1));
        if (ds_windows_wide_status((const unsigned char *)bytes + start * sizeof(uint16_t), units - start) != DS_WIDE_OK) return false;
        while (start < units && unit_at(bytes, start)) ++start;
        ++start;
    }
    return false;
}
/** @brief Reject short, oversized, or inconsistent descriptor sizes before dereferencing offsets. */
bool ds_windows_descriptor_valid(size_t allocated, size_t returned, size_t declared, size_t minimum)
{
    return minimum <= declared && declared <= returned && returned <= allocated && allocated <= 1024U * 1024U;
}
/** @brief Require nonzero offsets to reference terminated strings outside the fixed header. */
bool ds_windows_string_valid(const void *bytes, size_t length, size_t minimum, size_t offset)
{
    return offset == 0 || (offset >= minimum && offset < length && memchr((const char *)bytes + offset, '\0', length - offset));
}
