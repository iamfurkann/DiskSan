/**
 * @file
 * @brief Replace proc and native-discovery boundaries to verify Linux usage without real devices.
 */
#include "disksan/target.h"
#include "disksan/discovery.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/magic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/vfs.h>
#include <unistd.h>

static size_t checks;
static unsigned scenario, discovery_calls, stat_calls, handles, backing_stats, backing_reads;
static bool tracked[4096], sysfs_fd[4096];
static FILE *mount_file, *swap_file, *backing_file;
/** @brief Assert synthetic behavior without exposing host metadata. */
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)
/** @brief Access libc cleanup only for disposable fixture descriptors. */
int __real_close(int fd);
/** @brief Access libc stream cleanup for disposable fixtures. */
int __real_fclose(FILE *stream);
/** @brief Access libc stream wrapping for disposable fixtures. */
FILE *__real_fdopen(int fd, const char *mode);
/** @brief Access ordinary temporary-file metadata without any native device. */
int __real_fstat(int fd, struct stat *st);
/** @brief Call the ordinary owned-string implementation outside the injected allocation fault. */
int __real_ds_strings_add(ds_strings *list, const char *value);
/** @brief Read disposable text fixtures outside the injected stream fault. */
int __real_fgetc(FILE *stream);
/** @brief Query libc stream error outside the injected stream fault. */
int __real_ferror(FILE *stream);
/** @brief Inject failure while retaining the loop backing relationship without modifying the production allocator. */
int __wrap_ds_strings_add(ds_strings *list, const char *value)
{
    return scenario == 20U ? -1 : __real_ds_strings_add(list, value);
}
/** @brief Simulate an unreadable metadata stream without reading actual kernel paths. */
int __wrap_fgetc(FILE *stream)
{
    return scenario == 21U ? EOF : __real_fgetc(stream);
}
/** @brief Pair the injected EOF with an I/O error rather than clean end-of-file. */
int __wrap_ferror(FILE *stream)
{
    return scenario == 21U ? 1 : __real_ferror(stream);
}
/** @brief Permit exactly the two proc paths and read-only flags, substituting ordinary fixtures. */
int __wrap_open(const char *path, int flags, ...)
{
    bool backing = !strcmp(path, "/sys/devices/virtual/block/loop0/loop/backing_file");
    CHECK(backing || !strcmp(path, "/proc/self/mountinfo") || !strcmp(path, "/proc/swaps"));
    CHECK(flags == (O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (scenario == 3U || scenario == 4U) { errno = scenario == 3U ? EACCES : ENOMEM; return -1; }
    FILE *source = backing ? backing_file : !strcmp(path, "/proc/swaps") ? swap_file : mount_file;
    if (backing) {
        ++backing_reads;
        if (scenario == 16U && backing_reads == 2U) { rewind(source); CHECK(fputs("/synthetic/changed\n", source) >= 0 && fflush(source) == 0); }
        rewind(source);
    }
    int fd = dup(fileno(source));
    CHECK(fd >= 0 && fd < 4096); tracked[fd] = true; sysfs_fd[fd] = backing; ++handles;
    return fd;
}
/** @brief Simulate regular proc files or reject a substituted block descriptor before reading. */
int __wrap_fstat(int fd, struct stat *st)
{
    int rc = __real_fstat(fd, st);
    if (!rc && scenario == 6U) st->st_mode = S_IFBLK;
    return rc;
}
/** @brief Require procfs identity, with a non-proc filesystem negative fixture. */
int __wrap_fstatfs(int fd, struct statfs *fs)
{
    CHECK(fd >= 0 && fd < 4096 && tracked[fd]);
    memset(fs, 0, sizeof(*fs));
    fs->f_type = scenario == 5U || (scenario == 13U && sysfs_fd[fd]) ? 0 : sysfs_fd[fd] ? SYSFS_MAGIC : PROC_SUPER_MAGIC;
    return 0;
}
/** @brief Inject fdopen allocation failure while retaining the caller's cleanup responsibility. */
FILE *__wrap_fdopen(int fd, const char *mode)
{
    CHECK(!strcmp(mode, "r"));
    if (scenario == 7U) { errno = ENOMEM; return NULL; }
    return __real_fdopen(fd, mode);
}
/** @brief Track explicit descriptor cleanup on validation or stream-construction failure. */
int __wrap_close(int fd)
{
    if (fd >= 0 && fd < 4096 && tracked[fd]) { tracked[fd] = false; CHECK(handles > 0); --handles; }
    return __real_close(fd);
}
/** @brief Track stream cleanup after parsing, including malformed and oversized records. */
int __wrap_fclose(FILE *stream)
{
    int fd = fileno(stream);
    if (fd >= 0 && fd < 4096 && tracked[fd]) { tracked[fd] = false; CHECK(handles > 0); --handles; }
    return __real_fclose(stream);
}
/** @brief Substitute swap-file or block identity and optional path replacement without opening it. */
int __wrap_stat(const char *path, struct stat *st)
{
    if (!strcmp(path, "/synthetic/backing")) {
        ++backing_stats; memset(st, 0, sizeof(*st));
        if (scenario == 14U) { errno = EACCES; return -1; }
        st->st_mode = scenario == 18U ? S_IFBLK : S_IFREG;
        st->st_dev = scenario == 19U ? makedev(7, 0) : makedev(8, 1); st->st_rdev = makedev(8, 1);
        st->st_ino = scenario == 15U ? backing_stats : 1U;
        return 0;
    }
    CHECK(!strcmp(path, "/synthetic/swap file"));
    ++stat_calls; memset(st, 0, sizeof(*st));
    if (scenario == 8U) { errno = EACCES; return -1; }
    st->st_mode = S_IFREG; st->st_dev = makedev(8, 1); st->st_ino = scenario == 9U ? stat_calls : 1U;
    return 0;
}
/** @brief Build a selected disk and a root/swap partition, optionally changing generation. */
int ds_discover(ds_inventory *inventory, const char *selector, char *error, size_t size)
{
    (void)error; (void)size; CHECK(selector == NULL); ++discovery_calls;
    for (unsigned i = 0; i < 2U; ++i) {
        ds_device d; ds_device_init(&d); d.platform = "linux"; d.role = i ? "partition" : "disk";
        d.has_devnum = true; d.major_number = 8; d.minor_number = i;
        const char *path = i ? "/synthetic/part" : "/synthetic/disk";
        CHECK(ds_text_offer(&d, &d.path, path, "fixture") == 0);
        CHECK(ds_text_offer(&d, &d.sysfs_path, path, "fixture") == 0);
        CHECK(ds_text_offer(&d, &d.device_id, scenario == 2U && discovery_calls > 1U && !i ? "changed" : path, "sysfs:syspath+diskseq") == 0);
        if (i) CHECK(ds_text_offer(&d, &d.parent, "/synthetic/disk", "fixture") == 0);
        CHECK(ds_inventory_add(inventory, &d) == 0); ds_device_free(&d);
    }
    if (scenario >= 12U) {
        ds_device d; ds_device_init(&d); d.platform = "linux"; d.role = "virtual";
        d.has_devnum = true; d.major_number = 7;
        CHECK(ds_text_offer(&d, &d.path, "/synthetic/loop0", "fixture") == 0);
        CHECK(ds_text_offer(&d, &d.sysfs_path, "/sys/devices/virtual/block/loop0", "fixture") == 0);
        CHECK(ds_text_offer(&d, &d.device_id, "loop0#diskseq=1", "sysfs:syspath+diskseq") == 0);
        CHECK(ds_inventory_add(inventory, &d) == 0); ds_device_free(&d);
    }
    return 0;
}
/** @brief Create an ordinary temporary proc-text fixture and rewind it for duplication. */
static FILE *fixture(const char *text)
{
    FILE *file = tmpfile(); CHECK(file != NULL);
    CHECK(fputs(text, file) >= 0 && fflush(file) == 0); rewind(file);
    return file;
}
/** @brief Find a positive usage fact in collected evidence. */
static bool positive(const ds_assessment *result, ds_check check)
{
    for (size_t i = 0; i < result->check_count; ++i)
        if (result->checks[i].check == check && result->checks[i].state == DS_TRUE) return true;
    return false;
}
/** @brief Exercise native failure, permission, substitution, parse and cleanup paths. */
int main(void)
{
    for (scenario = 0; scenario < 22U; ++scenario) {
        ds_device selected; ds_device_init(&selected); selected.platform = "linux"; selected.role = "disk";
        selected.has_devnum = true; selected.major_number = 8;
        CHECK(ds_text_offer(&selected, &selected.path, "/synthetic/disk", "fixture") == 0);
        CHECK(ds_text_offer(&selected, &selected.device_id, "/synthetic/disk", "sysfs:syspath+diskseq") == 0);
        mount_file = fixture(scenario == 10U ? "" : scenario >= 12U ? "1 1 7:0 / / rw - ext4 /dev/alias rw\n" : "1 1 8:1 / / rw - ext4 /dev/alias rw\nmalformed\n");
        swap_file = fixture("Filename Type Size Used Priority\n/synthetic/swap\\040file file 4096 0 -2\n");
        backing_file = fixture("/synthetic/backing\n");
        if (scenario == 17U) { CHECK(fseek(backing_file, 0, SEEK_END) == 0); for (unsigned i = 0; i < 5000U; ++i) CHECK(fputc('X', backing_file) != EOF); CHECK(fflush(backing_file) == 0); rewind(backing_file); }
        if (scenario == 11U) { CHECK(fseek(mount_file, 0, SEEK_END) == 0); for (unsigned i = 0; i < 20000U; ++i) CHECK(fputc('X', mount_file) != EOF); CHECK(fflush(mount_file) == 0); rewind(mount_file); }
        discovery_calls = stat_calls = handles = backing_stats = backing_reads = 0;
        ds_assessment result; ds_assessment_init(&result); char error[256] = {0};
        int rc = ds_collect_usage(&selected, &result, error, sizeof(error));
        CHECK(handles == 0U);
        if (scenario == 2U || scenario == 15U || scenario == 16U) CHECK(rc == 3);
        else if (scenario == 4U || scenario == 7U || scenario == 20U) CHECK(rc == 1);
        else {
            CHECK(rc == 0);
            bool readable = scenario != 3U && scenario != 5U && scenario != 6U && scenario != 21U;
            bool matched = readable && scenario != 10U && scenario != 13U && scenario != 14U && scenario != 17U && scenario != 19U;
            CHECK(positive(&result, DS_CHECK_MOUNTED) == matched);
            CHECK(positive(&result, DS_CHECK_SYSTEM) == matched);
            if (scenario >= 12U) CHECK(positive(&result, DS_CHECK_SLAVES) == matched);
            CHECK(positive(&result, DS_CHECK_SWAP) == (readable && scenario != 8U && scenario != 9U));
            CHECK(ds_assessment_evaluate(&result) == 0 && result.status != DS_INSPECTION_NO_BLOCKERS);
        }
        CHECK(fclose(mount_file) == 0 && fclose(swap_file) == 0 && fclose(backing_file) == 0); ds_device_free(&selected);
    }
    printf("Linux usage fault fixtures: %zu assertions passed.\n", checks);
    return 0;
}
