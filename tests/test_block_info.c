/**
 * @file
 * @brief Inject Linux syscall failures without opening any real device descriptor.
 */
/* Linux-only syscall fault injection; no real device descriptors are opened. */
#include "disksan/discovery.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

static int open_error, ioctl_error, calls, closes;
static bool wrong_type, wrong_number, bad_sector, stat_error;
static unsigned int checks;
/** @brief Count an assertion and terminate the test on failure. */
#define CHECK(expr) do { ++checks; if (!(expr)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); exit(1); } } while (0)

/** @brief Assert read-only open flags and return a synthetic descriptor or configured errno. */
int __wrap_open(const char *path, int flags, ...)
{
    CHECK(strcmp(path, "/dev/test-fixture") == 0);
    CHECK((flags & O_ACCMODE) == O_RDONLY);
    CHECK((flags & O_CLOEXEC) != 0 && (flags & O_NOFOLLOW) != 0 && (flags & O_NONBLOCK) != 0);
    if (open_error) { errno = open_error; return -1; }
    return 42;
}

/** @brief Return a controlled block identity or inject a stat failure for the fake descriptor. */
int __wrap_fstat(int fd, struct stat *st)
{
    CHECK(fd == 42);
    if (stat_error) { errno = ENODEV; return -1; }
    memset(st, 0, sizeof(*st));
    st->st_mode = wrong_type ? S_IFREG : S_IFBLK;
    st->st_rdev = makedev(8, wrong_number ? 1U : 0U);
    return 0;
}

/** @brief Enforce the three-query allowlist and inject capacity, sector, or errno responses. */
int __wrap_ioctl(int fd, unsigned long request, ...)
{
    CHECK(fd == 42);
    CHECK(request == BLKGETSIZE64 || request == BLKSSZGET || request == BLKPBSZGET);
    ++calls;
    if (ioctl_error) { errno = ioctl_error; return -1; }
    va_list args;
    va_start(args, request);
    void *value = va_arg(args, void *);
    va_end(args);
    if (request == BLKGETSIZE64) *(uint64_t *)value = UINT64_C(4294967296);
    if (request == BLKSSZGET) *(int *)value = bad_sector ? 0 : 512;
    if (request == BLKPBSZGET) *(unsigned int *)value = bad_sector ? 0U : 4096U;
    return 0;
}

/** @brief Count closure of the synthetic descriptor without touching real file descriptors. */
int __wrap_close(int fd) { CHECK(fd == 42); ++closes; return 0; }

/** @brief Check whether the probe recorded the expected warning code. */
static bool warns(const ds_device *d, const char *code)
{
    for (size_t i = 0; i < d->warnings.count; ++i)
        if (strcmp(d->warnings.items[i], code) == 0) return true;
    return false;
}

/** @brief Create a fixture record and reset all syscall fault controls for the next scenario. */
static ds_device setup(void)
{
    ds_device d;
    ds_device_init(&d);
    d.major_number = 8;
    CHECK(ds_text_offer(&d, &d.path, "/dev/test-fixture", "fixture") == 0);
    open_error = ioctl_error = calls = closes = 0;
    wrong_type = wrong_number = bad_sector = stat_error = false;
    return d;
}

/** @brief Verify probe success, permission failures, identity changes, and invalid sector metadata. */
int main(void)
{
    ds_device d = setup();
    CHECK(ds_linux_probe(&d) == 0);
    CHECK(calls == 3 && closes == 1);
    CHECK(d.size_bytes.known && d.size_bytes.value == UINT64_C(4294967296));
    CHECK(d.logical_sector_size.value == 512 && d.physical_sector_size.value == 4096);
    ds_device_free(&d);
    const int errors[] = {EACCES, EPERM, ENOENT, ENODEV, ENXIO, ELOOP};
    const char *warnings[] = {"devnode_permission_denied", "devnode_permission_denied", "device_disappeared", "device_disappeared", "device_disappeared", "devnode_open_failed"};
    for (size_t i = 0; i < sizeof(errors) / sizeof(*errors); ++i) {
        d = setup(); open_error = errors[i];
        CHECK(ds_linux_probe(&d) == (errors[i] == ENOENT || errors[i] == ENODEV || errors[i] == ENXIO ? 1 : 0) && !d.size_bytes.known);
        CHECK(calls == 0 && closes == 0 && warns(&d, warnings[i]));
        ds_device_free(&d);
    }
    const int ioctl_errors[] = {EACCES, ENOTTY, EOPNOTSUPP, ENODEV, EIO};
    const char *ioctl_warnings[] = {"ioctl_permission_denied", "ioctl_unsupported", "ioctl_unsupported", "device_disappeared", "ioctl_failed"};
    for (size_t i = 0; i < sizeof(ioctl_errors) / sizeof(*ioctl_errors); ++i) {
        d = setup(); ioctl_error = ioctl_errors[i];
        CHECK(ds_linux_probe(&d) == (ioctl_errors[i] == ENODEV ? 1 : 0) && !d.size_bytes.known);
        CHECK(closes == 1 && warns(&d, ioctl_warnings[i]));
        CHECK(ds_number_offer(&d, &d.size_bytes, 2048, "sysfs:size*512") == 0);
        CHECK(d.size_bytes.known && d.size_bytes.value == 2048);
        ds_device_free(&d);
    }
    for (int i = 0; i < 3; ++i) {
        d = setup();
        wrong_type = i == 0; wrong_number = i == 1; stat_error = i == 2;
        CHECK(ds_linux_probe(&d) == 1 && calls == 0 && closes == 1);
        CHECK(warns(&d, "device_identity_changed"));
        ds_device_free(&d);
    }
    d = setup(); bad_sector = true;
    CHECK(ds_linux_probe(&d) == 0);
    CHECK(!d.logical_sector_size.known && !d.physical_sector_size.known);
    CHECK(warns(&d, "invalid_sector_size"));
    ds_device_free(&d);
    printf("%u Linux block-probe assertions passed.\n", checks);
    return 0;
}
