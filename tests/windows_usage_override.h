/** @file
 * @brief Substitute Windows volume queries only in the separately compiled native fault object.
 */
#ifndef DISKSAN_WINDOWS_USAGE_OVERRIDE_H
#define DISKSAN_WINDOWS_USAGE_OVERRIDE_H
#include <windows.h>
#include <winioctl.h>
/** @brief Supply the bounded synthetic shared system Windows-directory string. */
UINT WINAPI ds_test_windows_directory(LPWSTR path, UINT size);
/** @brief Supply a synthetic volume root. */
BOOL WINAPI ds_test_volume_path(LPCWSTR path, LPWSTR root, DWORD size);
/** @brief Supply a synthetic volume GUID name. */
BOOL WINAPI ds_test_volume_name(LPCWSTR root, LPWSTR volume, DWORD size);
/** @brief Create a fake volume handle while asserting zero desired access. */
HANDLE WINAPI ds_test_volume_open(LPCWSTR path, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security, DWORD disposition, DWORD flags, HANDLE template_file);
/** @brief Permit only the extent metadata control code and inject malformed responses. */
BOOL WINAPI ds_test_volume_query(HANDLE handle, DWORD code, LPVOID input, DWORD input_size, LPVOID output, DWORD output_size, LPDWORD returned, LPOVERLAPPED overlapped);
/** @brief Track fake volume-handle closure. */
BOOL WINAPI ds_test_handle_close(HANDLE handle);
/** @brief Begin synthetic GUID enumeration without contacting any device. */
HANDLE WINAPI ds_test_volume_first(LPWSTR name, DWORD size);
/** @brief End or fail synthetic GUID enumeration. */
BOOL WINAPI ds_test_volume_next(HANDLE search, LPWSTR name, DWORD size);
/** @brief Track enumeration-handle closure. */
BOOL WINAPI ds_test_volume_close(HANDLE search);
/** @brief Supply bounded valid or malformed mount-path MULTI_SZ. */
BOOL WINAPI ds_test_volume_paths(LPCWSTR volume, LPWSTR paths, DWORD size, PDWORD returned);
/** @brief Return the injected enumeration error code. */
DWORD WINAPI ds_test_last_error(void);
#define GetSystemWindowsDirectoryW ds_test_windows_directory
#define GetVolumePathNameW ds_test_volume_path
#define GetVolumeNameForVolumeMountPointW ds_test_volume_name
#define CreateFileW ds_test_volume_open
#define DeviceIoControl ds_test_volume_query
#define CloseHandle ds_test_handle_close
#define FindFirstVolumeW ds_test_volume_first
#define FindNextVolumeW ds_test_volume_next
#define FindVolumeClose ds_test_volume_close
#define GetVolumePathNamesForVolumeNameW ds_test_volume_paths
#define GetLastError ds_test_last_error
#endif
