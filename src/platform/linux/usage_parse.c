/**
 * @file
 * @brief Decode proc mount/swap metadata defensively without filesystem or device access.
 */
#include "usage_parse.h"
#include <limits.h>
#include <string.h>

/** @brief Copy and split one bounded ASCII-delimited line, rejecting embedded NUL and overflow. */
static size_t tokens(const char *line, size_t length, char *buffer, char **parts, size_t capacity)
{
    if (!line || !length || length > DS_PROC_LINE_LIMIT || memchr(line, '\0', length)) return 0;
    memcpy(buffer, line, length); buffer[length] = '\0';
    if (length && buffer[length - 1U] == '\n') buffer[--length] = '\0';
    for (size_t i = 0; i < length; ++i)
        if ((unsigned char)buffer[i] < 32U && buffer[i] != '\t') return 0;
    size_t count = 0;
    char *p = buffer;
    while (*p) {
        while (*p == ' ' || *p == '\t') ++p;
        if (!*p) break;
        if (count == capacity) return 0;
        parts[count++] = p;
        while (*p && *p != ' ' && *p != '\t') ++p;
        if (*p) *p++ = '\0';
    }
    return count;
}
/** @brief Parse unsigned kernel identifiers without accepting signs or integer overflow. */
static bool number(const char *text, unsigned int *out)
{
    uint64_t n;
    if (!ds_parse_u64(text, &n) || n > UINT_MAX) return false;
    *out = (unsigned int)n;
    return true;
}
/** @brief Decode only kernel-escaped whitespace/backslash, rejecting malformed or oversized paths. */
static bool path_decode(const char *text, char *out)
{
    size_t n = 0;
    while (*text) {
        unsigned char value = (unsigned char)*text++;
        if (value == '\\') {
            if (!text[0] || !text[1] || !text[2]) return false;
            if (!strncmp(text, "040", 3)) value = ' ';
            else if (!strncmp(text, "011", 3)) value = '\t';
            else if (!strncmp(text, "012", 3)) value = '\n';
            else if (!strncmp(text, "134", 3)) value = '\\';
            else return false;
            text += 3;
        }
        if (n == DS_TEXT_LIMIT) return false;
        out[n++] = (char)value;
    }
    out[n] = '\0';
    return n != 0U && out[0] == '/';
}
/** @brief Parse known fixed mount fields and ignore bounded forward-compatible optional tags. */
int ds_parse_mountinfo(const char *line, size_t length, ds_mount_record *out)
{
    memset(out, 0, sizeof(*out));
    char buffer[DS_PROC_LINE_LIMIT + 1U], *parts[128];
    size_t count = tokens(line, length, buffer, parts, sizeof(parts) / sizeof(parts[0]));
    if (count < 10U) return -1;
    size_t separator = 6U;
    while (separator < count && strcmp(parts[separator], "-")) ++separator;
    if (separator + 4U != count) return -1;
    char *colon = strchr(parts[2], ':');
    if (!colon || strchr(colon + 1, ':')) return -1;
    *colon = '\0';
    ds_mount_record record = {0};
    if (!number(parts[0], &record.mount_id) || !record.mount_id ||
        !number(parts[1], &record.parent_id) ||
        !number(parts[2], &record.major_number) || !number(colon + 1, &record.minor_number) ||
        !path_decode(parts[3], record.root) || !path_decode(parts[4], record.mount_point)) return -1;
    *out = record;
    return 0;
}
/** @brief Parse priorities through an unsigned magnitude to avoid signed minimum overflow. */
static bool priority_number(const char *text, int *out)
{
    bool negative = text[0] == '-';
    uint64_t n;
    if (!ds_parse_u64(text + (negative ? 1 : 0), &n)) return false;
    uint64_t limit = negative ? (uint64_t)INT_MAX + 1U : (uint64_t)INT_MAX;
    if (n > limit) return false;
    *out = negative ? n == limit ? INT_MIN : -(int)n : (int)n;
    return true;
}
/** @brief Parse a swap row without deriving its backing device from a brittle path name. */
int ds_parse_swap(const char *line, size_t length, ds_swap_record *out)
{
    memset(out, 0, sizeof(*out));
    char buffer[DS_PROC_LINE_LIMIT + 1U], *parts[6];
    if (tokens(line, length, buffer, parts, sizeof(parts) / sizeof(parts[0])) != 5U) return -1;
    ds_swap_record record = {0};
    if (strcmp(parts[1], "file") && strcmp(parts[1], "partition")) return -1;
    record.file = !strcmp(parts[1], "file");
    if (!path_decode(parts[0], record.path) || !ds_parse_u64(parts[2], &record.size_kib) ||
        !ds_parse_u64(parts[3], &record.used_kib) || record.used_kib > record.size_kib ||
        !priority_number(parts[4], &record.priority)) return -1;
    *out = record;
    return 0;
}
/** @brief Accept only the complete five-column kernel swap header. */
bool ds_swaps_header(const char *line, size_t length)
{
    char buffer[DS_PROC_LINE_LIMIT + 1U], *parts[6];
    if (tokens(line, length, buffer, parts, sizeof(parts) / sizeof(parts[0])) != 5U) return false;
    return !strcmp(parts[0], "Filename") && !strcmp(parts[1], "Type") && !strcmp(parts[2], "Size") &&
           !strcmp(parts[3], "Used") && !strcmp(parts[4], "Priority");
}
