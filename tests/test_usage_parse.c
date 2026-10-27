/**
 * @file
 * @brief Test Linux usage parsers on any host using bounded malformed and escaped fixtures.
 */
#include "../src/platform/linux/usage_parse.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static size_t checks;
/** @brief Count synthetic assertions and report a source-line failure. */
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)
/** @brief Parse valid bind/root/escaped mount records while retaining numeric device identity. */
static void valid_mounts(void)
{
    const char *line = "36 35 8:17 /dir\\040space /mount\\011tab\\012line\\134slash rw shared:2 unknown-tag:9 - ext4 /dev/alias rw\n";
    ds_mount_record m;
    CHECK(ds_parse_mountinfo(line, strlen(line), &m) == 0);
    CHECK(m.mount_id == 36U && m.parent_id == 35U && m.major_number == 8U && m.minor_number == 17U);
    CHECK(!strcmp(m.root, "/dir space") && !strcmp(m.mount_point, "/mount\ttab\nline\\slash"));
    line = "1 1 0:5 / / rw - tmpfs none rw";
    CHECK(ds_parse_mountinfo(line, strlen(line), &m) == 0 && !strcmp(m.mount_point, "/"));
    /* Repeated mount points with different IDs are valid and cannot be deduplicated by path. */
    line = "2 1 8:2 / / rw - ext4 /dev/synthetic rw";
    CHECK(ds_parse_mountinfo(line, strlen(line), &m) == 0 && m.mount_id == 2U && m.minor_number == 2U);
}
/** @brief Reject malformed IDs, delimiters, escape sequences, NULs and oversized records. */
static void invalid_mounts(void)
{
    const char *cases[] = {"", "1 1 8:1 / / rw", "-1 1 8:1 / / rw - ext4 none rw", "0 1 8:1 / / rw - ext4 none rw",
        "1 1 +8:1 / / rw - ext4 none rw", "1 1 8:-1 / / rw - ext4 none rw", "1 1 8:1:2 / / rw - ext4 none rw",
        "1 1 4294967296:1 / / rw - ext4 none rw", "1 1 8:1 / / rw ext4 none rw", "1 1 8:1 relative / rw - ext4 none rw",
        "1 1 8:1 / /bad\\0 rw - ext4 none rw", "1 1 8:1 / /bad\\000 rw - ext4 none rw", "1 1 8:1 / / rw - ext4 none rw extra",
        "1 1 8:1 / / rw\n - ext4 none rw", "1 1 8:1 / / rw - ext4 none rw\r\n", "1 1 8\v:1 / / rw - ext4 none rw"};
    ds_mount_record m, empty = {0};
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        memset(&m, 'X', sizeof(m));
        CHECK(ds_parse_mountinfo(cases[i], strlen(cases[i]), &m) == -1);
        CHECK(!memcmp(&m, &empty, sizeof(m)));
    }
    const char embedded[] = "1 1 8:1 / /\0 rw - ext4 none rw";
    CHECK(ds_parse_mountinfo(embedded, sizeof(embedded) - 1U, &m) == -1);
    char huge[DS_PROC_LINE_LIMIT + 2U]; memset(huge, 'A', sizeof(huge));
    CHECK(ds_parse_mountinfo(huge, sizeof(huge), &m) == -1);
    CHECK(ds_parse_mountinfo(NULL, 5, &m) == -1);
    char long_path[DS_TEXT_LIMIT + 100U];
    memcpy(long_path, "1 1 8:1 / /", 11); memset(long_path + 11, 'X', DS_TEXT_LIMIT);
    memcpy(long_path + 11U + DS_TEXT_LIMIT, " rw - ext4 none rw", 18);
    CHECK(ds_parse_mountinfo(long_path, 11U + DS_TEXT_LIMIT + 17U, &m) == -1);
}
/** @brief Validate swap headers, escaped file paths and exact unsigned/signed boundaries. */
static void swaps(void)
{
    const char *header = "Filename\t\tType\tSize\tUsed\tPriority\n";
    CHECK(ds_swaps_header(header, strlen(header)));
    CHECK(!ds_swaps_header("Filename Type Size Used", 23));
    const char *line = "/swap\\040file file 18446744073709551615 0 -2\n";
    ds_swap_record r, empty = {0};
    CHECK(ds_parse_swap(line, strlen(line), &r) == 0 && r.file && r.size_kib == UINT64_MAX && r.priority == -2);
    CHECK(!strcmp(r.path, "/swap file"));
    char boundary[100]; (void)snprintf(boundary, sizeof(boundary), "/dev/synthetic partition 10 10 %d", INT_MIN);
    CHECK(ds_parse_swap(boundary, strlen(boundary), &r) == 0 && !r.file && r.priority == INT_MIN);
    (void)snprintf(boundary, sizeof(boundary), "/dev/synthetic partition 10 0 %d", INT_MAX);
    CHECK(ds_parse_swap(boundary, strlen(boundary), &r) == 0 && r.priority == INT_MAX);
    const char *cases[] = {"", "/a file 1 2 0", "/a file 18446744073709551616 0 0", "/a file 1 0 +1",
        "/a file 1 0 -2147483649", "/a file 1 0 2147483648", "/a file 1 0 -", "/a odd 1 0 1", "a file 1 0 1",
        "/a\\999 file 1 0 1", "/a file -1 0 1", "/a file 1 -1 1", "/a file 1 0 1 extra", "/a file 1 0", "/a\\ file 1 0 1", "/a file 1\f 0 0"};
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        memset(&r, 'X', sizeof(r));
        CHECK(ds_parse_swap(cases[i], strlen(cases[i]), &r) == -1);
        CHECK(!memcmp(&r, &empty, sizeof(r)));
    }
}
/** @brief Exercise all byte values and every prefix of valid records under sanitizers. */
static void truncation_and_bytes(void)
{
    const char *line = "1 1 8:1 / / rw - ext4 none rw\n";
    ds_mount_record m;
    for (size_t n = 0; n <= strlen(line); ++n) {
        int rc = ds_parse_mountinfo(line, n, &m);
        CHECK(rc == 0 || rc == -1);
    }
    char input[80];
    for (unsigned byte = 0; byte < 256U; ++byte) {
        memset(input, (int)byte, sizeof(input));
        CHECK(ds_parse_mountinfo(input, sizeof(input), &m) == -1);
        ds_swap_record s;
        CHECK(ds_parse_swap(input, sizeof(input), &s) == -1);
    }
}
/** @brief Run fixture-only usage parsing coverage without reading host proc metadata. */
int main(void)
{
    valid_mounts(); invalid_mounts(); swaps(); truncation_and_bytes();
    printf("Usage parsers: %zu assertions passed.\n", checks);
    return 0;
}
