/** @file
 * @brief Test Linux record revalidation using ordinary temporary sysfs fixtures and fake udev metadata.
 */
#include "disksan/discovery.h"
#include "../src/platform/linux/native.h"
#include <libudev.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
static char root[4096];
static unsigned int checks;
static int change;
/** @brief Count assertions and stop on a failing fixture invariant. */
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)
/** @brief Write only an ordinary metadata fixture file inside the test's unique temporary directory. */
static void attribute(const char *name, const char *value)
{
    char path[8192]; (void)snprintf(path, sizeof(path), "%s/%s", root, name);
    FILE *out = fopen(path, "w"); CHECK(out != NULL);
    CHECK(fputs(value, out) >= 0); CHECK(fclose(out) == 0);
}
/** @brief Return the ordinary fixture directory as a borrowed synthetic sysfs path. */
const char *__wrap_udev_device_get_syspath(struct udev_device *u) { (void)u; return root; }
/** @brief Limit the synthetic record to the block subsystem. */
const char *__wrap_udev_device_get_subsystem(struct udev_device *u) { (void)u; return "block"; }
/** @brief Provide a synthetic Unix device number without creating a device node. */
dev_t __wrap_udev_device_get_devnum(struct udev_device *u) { (void)u; return makedev(8, 0); }
/** @brief Leave the device node absent so no real path can be opened. */
const char *__wrap_udev_device_get_devnode(struct udev_device *u) { (void)u; return NULL; }
/** @brief Classify the synthetic record as a whole disk. */
const char *__wrap_udev_device_get_devtype(struct udev_device *u) { (void)u; return "disk"; }
/** @brief Leave optional udev properties unavailable. */
const char *__wrap_udev_device_get_property_value(struct udev_device *u, const char *key) { (void)u; (void)key; return NULL; }
/** @brief End synthetic native ancestry immediately. */
struct udev_device *__wrap_udev_device_get_parent(struct udev_device *u) { (void)u; return NULL; }
/** @brief Supply no by-id aliases in the ordinary-file metadata fixture. */
struct udev_list_entry *__wrap_udev_device_get_devlinks_list_entry(struct udev_device *u) { (void)u; return NULL; }
/** @brief Change only fixture metadata at the probe boundary to simulate replacement or disappearance. */
int __wrap_ds_linux_probe(ds_device *d)
{
    (void)d;
    if (change == 1) attribute("diskseq", "2\n");
    if (change == 2) attribute("dev", "8:1\n");
    if (change == 3) return 1;
    return 0;
}
/** @brief Check current, missing, invalid, and changed generation metadata with deterministic cleanup. */
int main(void)
{
    const char *tmp = getenv("TMPDIR"); if (!tmp) tmp = "/tmp";
    int length = snprintf(root, sizeof(root), "%s/disksan-metadata-XXXXXX", tmp);
    CHECK(length > 0 && (size_t)length < sizeof(root)); CHECK(mkdtemp(root) != NULL);
    max_align_t fake;
    for (int scenario = 0; scenario < 6; ++scenario) {
        attribute("dev", "8:0\n"); attribute("size", "8\n");
        char path[8192]; (void)snprintf(path, sizeof(path), "%s/diskseq", root);
        if (scenario == 4) CHECK(unlink(path) == 0);
        else attribute("diskseq", scenario == 5 ? "invalid\n" : "1\n");
        change = scenario;
        ds_device d; ds_device_init(&d);
        int rc = ds_linux_collect((struct udev_device *)(void *)&fake, &d);
        CHECK(rc == (scenario >= 1 && scenario <= 3 ? 1 : 0));
        if (!rc) {
            CHECK(d.size_bytes.known && d.size_bytes.value == 4096);
            if (!scenario) CHECK(strstr(d.device_id.value, "#diskseq=1") != NULL);
            else {
                CHECK(!strcmp(d.device_id.value, root));
                bool warned = false;
                for (size_t i = 0; i < d.warnings.count; ++i) if (!strcmp(d.warnings.items[i], "identity_generation_unavailable")) warned = true;
                CHECK(warned);
            }
        }
        ds_device_free(&d);
    }
    const char *files[] = {"dev", "size", "diskseq"};
    for (size_t i = 0; i < 3; ++i) {
        char path[8192]; (void)snprintf(path, sizeof(path), "%s/%s", root, files[i]); CHECK(unlink(path) == 0);
    }
    CHECK(rmdir(root) == 0);
    printf("%u Linux metadata assertions passed; 6 synthetic revalidation scenarios.\n", checks);
    return 0;
}
