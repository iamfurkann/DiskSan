/** @file
 * @brief Exercise macOS usage cleanup, allocation failures and malformed responses with synthetic metadata.
 */
#include "macos_usage_override.h"
#undef DASessionCreate
#undef DADiskCreateFromBSDName
#undef DADiskCopyDescription
#undef CFRelease
#undef CFURLGetFileSystemRepresentation
#undef getfsstat
#undef stat
#undef calloc
#undef free
#include "disksan/target.h"
#include "disksan/discovery.h"
#include <errno.h>
#include <string.h>

static unsigned scenario, references, allocations, discoveries, checks;
/** @brief Stop at the first violated ownership or conservative-result contract. */
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)
/** @brief Create an ordinary Core Foundation fixture with tracked caller ownership. */
static CFStringRef object(void)
{
    CFStringRef value = CFStringCreateWithCString(NULL, "fixture", kCFStringEncodingUTF8);
    CHECK(value != NULL); ++references; return value;
}
/** @brief Simulate an unavailable session without contacting Disk Arbitration. */
DASessionRef ds_test_session(CFAllocatorRef allocator)
{
    (void)allocator; return scenario == 3U ? NULL : (DASessionRef)object();
}
/** @brief Check the selected path and create a synthetic owned disk reference. */
DADiskRef ds_test_disk(CFAllocatorRef allocator, DASessionRef session, const char *name)
{
    (void)allocator; CHECK(session != NULL && !strcmp(name, "/dev/synthetic1"));
    return scenario == 4U ? NULL : (DADiskRef)object();
}
/** @brief Create a volume-path description, including wrong-type and missing-property fixtures. */
CFDictionaryRef ds_test_description(DADiskRef disk)
{
    CHECK(disk != NULL); if (scenario == 5U) return NULL;
    CFMutableDictionaryRef result = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CHECK(result != NULL); ++references;
    if (scenario != 13U) {
        CFTypeRef path = scenario == 6U ? (CFTypeRef)CFStringCreateWithCString(NULL, "/", kCFStringEncodingUTF8) :
            (CFTypeRef)CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8 *)"/", 1, true);
        CHECK(path != NULL); CFDictionarySetValue(result, kDADiskDescriptionVolumePathKey, path); CFRelease(path);
    }
    return result;
}
/** @brief Require matching release for every copied native object. */
void ds_test_release(CFTypeRef value)
{
    CHECK(value != NULL && references > 0U); --references; CFRelease(value);
}
/** @brief Fail URL conversion or use a pure in-memory Core Foundation fixture. */
Boolean ds_test_path(CFURLRef url, Boolean absolute, UInt8 *buffer, CFIndex size)
{
    CHECK(size > 0 && absolute); return scenario == 7U ? false : CFURLGetFileSystemRepresentation(url, absolute, buffer, size);
}
/** @brief Require cached metadata flags and inject bounded mount count/string failures. */
int ds_test_mounts(struct statfs *buffer, int size, int flags)
{
    CHECK(flags == MNT_NOWAIT);
    if (!buffer) { CHECK(size == 0); return scenario == 8U ? 0 : scenario == 9U ? 1024 : 1; }
    CHECK(size >= (int)sizeof(*buffer));
    if (scenario == 10U) return -1;
    memset(buffer, 0, sizeof(*buffer));
    if (scenario == 11U) memset(buffer->f_mntfromname, 'X', sizeof(buffer->f_mntfromname));
    else (void)snprintf(buffer->f_mntfromname, sizeof(buffer->f_mntfromname), "/dev/synthetic1");
    (void)snprintf(buffer->f_mntonname, sizeof(buffer->f_mntonname), "/");
    return 1;
}
/** @brief Inject inaccessible nodes; otherwise return a synthetic related block identifier. */
int ds_test_stat(const char *path, struct stat *value)
{
    CHECK(!strcmp(path, "/dev/synthetic1"));
    if (scenario == 12U) { errno = EACCES; return -1; }
    memset(value, 0, sizeof(*value)); value->st_mode = S_IFBLK;
    value->st_rdev = makedev(1, scenario == 13U ? 2 : 1); return 0;
}
/** @brief Fail mount-table allocation without changing any production allocator. */
void *ds_test_calloc(size_t count, size_t size)
{
    CHECK(count <= 1024U && size == sizeof(struct statfs));
    if (scenario == 2U) return NULL;
    void *result = calloc(count, size); CHECK(result != NULL); ++allocations; return result;
}
/** @brief Account for the collector's only heap-owned native response. */
void ds_test_free(void *pointer)
{
    CHECK(pointer != NULL && allocations > 0U); --allocations; free(pointer);
}
/** @brief Build a single known registry record or inject a changed identity in the closing snapshot. */
int ds_discover(ds_inventory *inventory, const char *selector, char *error, size_t size)
{
    (void)error; (void)size; CHECK(selector == NULL); ++discoveries;
    ds_device device; ds_device_init(&device); device.platform = "macos"; device.role = "disk";
    device.has_devnum = true; device.major_number = 1; device.minor_number = 1;
    CHECK(ds_text_offer(&device, &device.path, "/dev/synthetic1", "fixture") == 0);
    CHECK(ds_text_offer(&device, &device.device_id, scenario == 14U && discoveries > 1U ? "changed" : "fixture", "iokit:registry_entry_id") == 0);
    CHECK(ds_inventory_add(inventory, &device) == 0); ds_device_free(&device); return 0;
}
/** @brief Run native-boundary faults with ordinary memory only and require complete cleanup. */
int main(void)
{
    for (scenario = 0; scenario < 15U; ++scenario) {
        ds_inventory inventory = {0}; char error[256] = {0}; discoveries = 0;
        CHECK(ds_discover(&inventory, NULL, error, sizeof(error)) == 0); discoveries = 0;
        ds_assessment result; ds_assessment_init(&result);
        int rc = ds_collect_usage(&inventory.items[0], &result, error, sizeof(error));
        CHECK(references == 0U && allocations == 0U);
        CHECK(rc == (scenario == 2U ? 1 : scenario == 14U ? 3 : 0));
        if (!rc) {
            CHECK(ds_assessment_evaluate(&result) == 0 && result.status != DS_INSPECTION_NO_BLOCKERS);
            bool positive = false;
            for (size_t i = 0; i < result.check_count; ++i)
                positive |= result.checks[i].check == DS_CHECK_SYSTEM && result.checks[i].state == DS_TRUE;
            CHECK(positive == (scenario != 13U));
        }
        ds_inventory_free(&inventory);
    }
    printf("macOS synthetic usage faults: %u assertions passed.\n", checks); return 0;
}
