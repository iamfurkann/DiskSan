/** @file
 * @brief Test Windows volume-query bounds, zero-access flags, changes and handle cleanup with synthetic responses.
 */
#include "windows_usage_override.h"
#include "disksan/target.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

static unsigned scenario, handles, queries, checks;
static DWORD last_error;
static const WCHAR volume[] = L"\\\\?\\Volume{synthetic-fixture}\\";
/** @brief Fail a contract violation without opening a real native device. */
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)
/** @brief Copy only test-owned terminated literals into caller-supplied bounded storage. */
static void copy_wide(WCHAR *out, DWORD size, const WCHAR *text)
{
    size_t count = wcslen(text) + 1U; CHECK(count <= size); memcpy(out, text, count * sizeof(WCHAR));
}
/** @brief Return a valid directory, a missing directory or invalid UTF-16 before downstream use. */
UINT WINAPI ds_test_windows_directory(LPWSTR path, UINT size)
{
    if (scenario == 12U) return 0;
    copy_wide(path, size, L"C:\\Windows");
    if (scenario == 13U) path[0] = (WCHAR)0xd800U;
    return 10;
}
/** @brief Assert a validated source path and return a synthetic root. */
BOOL WINAPI ds_test_volume_path(LPCWSTR path, LPWSTR root, DWORD size)
{
    CHECK(!wcscmp(path, L"C:\\Windows")); copy_wide(root, size, L"C:\\"); return TRUE;
}
/** @brief Return a synthetic system-volume GUID path. */
BOOL WINAPI ds_test_volume_name(LPCWSTR root, LPWSTR name, DWORD size)
{
    CHECK(!wcscmp(root, L"C:\\")); copy_wide(name, size, volume); return TRUE;
}
/** @brief Reject any access escalation, creation flags or non-volume path. */
HANDLE WINAPI ds_test_volume_open(LPCWSTR path, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security, DWORD disposition, DWORD flags, HANDLE template_file)
{
    CHECK(access == 0U && share == (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE));
    CHECK(security == NULL && disposition == OPEN_EXISTING && flags == 0U && template_file == NULL);
    CHECK(wcslen(path) + 1U == wcslen(volume) && !wcsncmp(path, volume, wcslen(path)));
    if (scenario == 1U) return INVALID_HANDLE_VALUE;
    ++handles; return (HANDLE)(uintptr_t)2;
}
/** @brief Provide only extent metadata, including short/count/range errors and closing-query changes. */
BOOL WINAPI ds_test_volume_query(HANDLE handle, DWORD code, LPVOID input, DWORD input_size, LPVOID output, DWORD output_size, LPDWORD returned, LPOVERLAPPED overlapped)
{
    CHECK(handle == (HANDLE)(uintptr_t)2 && code == IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS);
    CHECK(input == NULL && input_size == 0U && overlapped == NULL);
    CHECK(output_size >= sizeof(VOLUME_DISK_EXTENTS) && output_size < 4096U); ++queries;
    if (scenario == 3U || (scenario == 9U && queries == 2U)) return FALSE;
    VOLUME_DISK_EXTENTS *data = output; memset(output, 0, output_size);
    data->NumberOfDiskExtents = scenario == 5U ? 65U : 1U;
    data->Extents[0].DiskNumber = scenario == 8U && queries == 2U ? 1U : 0U;
    data->Extents[0].StartingOffset.QuadPart = scenario == 6U ? -1 : scenario == 7U ? INT64_MAX : 0;
    data->Extents[0].ExtentLength.QuadPart = 4096;
    *returned = scenario == 4U ? 1U : (DWORD)sizeof(VOLUME_DISK_EXTENTS);
    return TRUE;
}
/** @brief Track exact volume-handle ownership on every query path. */
BOOL WINAPI ds_test_handle_close(HANDLE handle)
{
    CHECK(handle == (HANDLE)(uintptr_t)2 && handles > 0U); --handles; return TRUE;
}
/** @brief Create one enumeration handle or simulate a failed/malformed initial response. */
HANDLE WINAPI ds_test_volume_first(LPWSTR name, DWORD size)
{
    if (scenario == 2U) return INVALID_HANDLE_VALUE;
    copy_wide(name, size, volume);
    if (scenario == 14U) for (DWORD i = 0; i < size; ++i) name[i] = L'X';
    ++handles; return (HANDLE)(uintptr_t)1;
}
/** @brief Stop enumeration with normal EOF or an explicit native error. */
BOOL WINAPI ds_test_volume_next(HANDLE search, LPWSTR name, DWORD size)
{
    (void)name; (void)size; CHECK(search == (HANDLE)(uintptr_t)1);
    last_error = scenario == 15U ? ERROR_ACCESS_DENIED : ERROR_NO_MORE_FILES; return FALSE;
}
/** @brief Release the enumeration handle even when a volume query fails or changes. */
BOOL WINAPI ds_test_volume_close(HANDLE search)
{
    CHECK(search == (HANDLE)(uintptr_t)1 && handles > 0U); --handles; return TRUE;
}
/** @brief Return mounted paths or malformed termination/UTF-16 without native calls. */
BOOL WINAPI ds_test_volume_paths(LPCWSTR name, LPWSTR paths, DWORD size, PDWORD returned)
{
    CHECK(!wcscmp(name, volume) && size >= 5U);
    copy_wide(paths, size, L"C:\\"); paths[4] = scenario == 10U ? L'X' : L'\0';
    if (scenario == 11U) paths[0] = (WCHAR)0xd800U;
    *returned = 5U; return TRUE;
}
/** @brief Return the deterministic error associated with enumeration. */
DWORD WINAPI ds_test_last_error(void)
{
    return last_error;
}
/** @brief Inspect a typed positive check without assuming any missing check is false. */
static bool positive(const ds_assessment *result, ds_check check)
{
    for (size_t i = 0; i < result->check_count; ++i)
        if (result->checks[i].check == check && result->checks[i].state == DS_TRUE) return true;
    return false;
}
/** @brief Run every response fault and verify no real handles, heap storage or erase authorization exists. */
int main(void)
{
    for (scenario = 0; scenario < 16U; ++scenario) {
        ds_device device; ds_device_init(&device); device.platform = "windows"; device.role = "disk";
        CHECK(ds_text_offer(&device, &device.path, "\\\\.\\PhysicalDrive0", "fixture") == 0);
        ds_assessment result; ds_assessment_init(&result); char error[256] = {0}; queries = handles = 0;
        int rc = ds_collect_usage(&device, &result, error, sizeof(error));
        CHECK(handles == 0U && queries <= 2U);
        CHECK(rc == (scenario == 8U || scenario == 9U ? 3 : 0));
        if (!rc) {
            bool matched = scenario == 0U || scenario == 10U || scenario == 11U || scenario == 12U || scenario == 13U || scenario == 15U;
            CHECK(positive(&result, DS_CHECK_MOUNTED) == (matched && scenario != 10U && scenario != 11U));
            CHECK(positive(&result, DS_CHECK_SYSTEM) == (matched && scenario != 12U && scenario != 13U));
            CHECK(ds_assessment_evaluate(&result) == 0 && result.status != DS_INSPECTION_NO_BLOCKERS);
        }
        ds_device_free(&device);
    }
    printf("Windows synthetic usage faults: %u assertions passed.\n", checks); return 0;
}
