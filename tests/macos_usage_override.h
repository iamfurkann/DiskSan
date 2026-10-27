/** @file
 * @brief Substitute native usage boundaries only in the separately compiled macOS fault object.
 */
#ifndef DISKSAN_MAC_USAGE_OVERRIDE_H
#define DISKSAN_MAC_USAGE_OVERRIDE_H
#include <DiskArbitration/DiskArbitration.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
/** @brief Create a synthetic owned session object, or inject absence. */
DASessionRef ds_test_session(CFAllocatorRef allocator);
/** @brief Create a synthetic owned disk object without a native query. */
DADiskRef ds_test_disk(CFAllocatorRef allocator, DASessionRef session, const char *name);
/** @brief Return a synthetic volume description with controlled type and ownership. */
CFDictionaryRef ds_test_description(DADiskRef disk);
/** @brief Track all copied native references and release real fixture objects. */
void ds_test_release(CFTypeRef value);
/** @brief Inject bounded URL conversion failure. */
Boolean ds_test_path(CFURLRef url, Boolean absolute, UInt8 *buffer, CFIndex size);
/** @brief Supply a cached synthetic mount table and native error cases. */
int ds_test_mounts(struct statfs *buffer, int size, int flags);
/** @brief Return synthetic block-node metadata without accessing a device. */
int ds_test_stat(const char *path, struct stat *value);
/** @brief Track and fail the collector's only heap allocation. */
void *ds_test_calloc(size_t count, size_t size);
/** @brief Verify the mount-table allocation is released on every path. */
void ds_test_free(void *pointer);
#define DASessionCreate ds_test_session
#define DADiskCreateFromBSDName ds_test_disk
#define DADiskCopyDescription ds_test_description
#define CFRelease ds_test_release
#define CFURLGetFileSystemRepresentation ds_test_path
#define getfsstat ds_test_mounts
#define stat(path, value) ds_test_stat(path, value)
#define calloc ds_test_calloc
#define free ds_test_free
#endif
