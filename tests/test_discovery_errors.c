/**
 * @file
 * @brief Inject libudev boundary failures without requiring or changing real devices.
 */
/* Inject libudev boundary failures without requiring or changing real devices. */
#include "disksan/discovery.h"
#include <errno.h>
#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** @brief Select the libudev boundary outcome injected by this test. */
enum scenario { INIT_FAIL, ENUM_FAIL, MATCH_FAIL, SCAN_FAIL, EMPTY, DISAPPEARED, DEVICE_OOM };
static enum scenario scenario;
static max_align_t entry_storage;
static unsigned int checks;
/** @brief Count an assertion and terminate the test on failure. */
#define CHECK(expr) do { ++checks; if (!(expr)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); exit(1); } } while (0)

/** @brief Link to the original libudev context initializer for wrapper delegation. */
struct udev *__real_udev_new(void);
/** @brief Link to the original libudev enumeration allocator. */
struct udev_enumerate *__real_udev_enumerate_new(struct udev *udev);
/** @brief Link to the original libudev subsystem filter implementation. */
int __real_udev_enumerate_add_match_subsystem(struct udev_enumerate *enumeration, const char *subsystem);

/** @brief Inject context allocation failure or delegate to the real libudev initializer. */
struct udev *__wrap_udev_new(void)
{
    if (scenario == INIT_FAIL) { errno = ENOMEM; return NULL; }
    return __real_udev_new();
}

/** @brief Inject enumeration allocation failure or create a real enumeration object. */
struct udev_enumerate *__wrap_udev_enumerate_new(struct udev *udev)
{
    if (scenario == ENUM_FAIL) { errno = ENOMEM; return NULL; }
    return __real_udev_enumerate_new(udev);
}

/** @brief Assert block-only discovery and optionally fail subsystem filter creation. */
int __wrap_udev_enumerate_add_match_subsystem(struct udev_enumerate *enumeration, const char *subsystem)
{
    CHECK(strcmp(subsystem, "block") == 0);
    return scenario == MATCH_FAIL ? -ENOMEM : __real_udev_enumerate_add_match_subsystem(enumeration, subsystem);
}

/** @brief Return a synthetic scan result without enumerating actual devices. */
int __wrap_udev_enumerate_scan_devices(struct udev_enumerate *enumeration)
{
    (void)enumeration;
    return scenario == SCAN_FAIL ? -EIO : 0;
}

/** @brief Return an empty list or one aligned synthetic list entry. */
struct udev_list_entry *__wrap_udev_enumerate_get_list_entry(struct udev_enumerate *enumeration)
{
    (void)enumeration;
    return scenario == EMPTY ? NULL : (struct udev_list_entry *)(void *)&entry_storage;
}

/** @brief Return the synthetic syspath after validating the fake entry address. */
const char *__wrap_udev_list_entry_get_name(struct udev_list_entry *entry)
{
    CHECK(entry == (struct udev_list_entry *)(void *)&entry_storage);
    return "/sys/fixture/removed-block-device";
}

/** @brief Terminate the single-entry synthetic enumeration. */
struct udev_list_entry *__wrap_udev_list_entry_get_next(struct udev_list_entry *entry)
{
    CHECK(entry == (struct udev_list_entry *)(void *)&entry_storage);
    return NULL;
}

/** @brief Simulate a disappeared device or allocation failure while resolving the fake syspath. */
struct udev_device *__wrap_udev_device_new_from_syspath(struct udev *udev, const char *syspath)
{
    (void)udev;
    CHECK(strcmp(syspath, "/sys/fixture/removed-block-device") == 0);
    errno = scenario == DEVICE_OOM ? ENOMEM : ENODEV;
    return NULL;
}

/** @brief Verify seven discovery boundary scenarios and their errors, warnings, and empty inventories. */
int main(void)
{
    for (int current = INIT_FAIL; current <= DEVICE_OOM; ++current) {
        scenario = (enum scenario)current;
        ds_inventory inventory = {0};
        char error[512] = {0};
        int rc = ds_discover(&inventory, NULL, error, sizeof(error));
        CHECK(rc == (scenario == EMPTY ? 0 : 1));
        CHECK(inventory.count == 0);
        CHECK((error[0] == '\0') == (scenario == EMPTY));
        if (scenario == DISAPPEARED) {
            CHECK(inventory.warnings.count == 1);
            CHECK(strcmp(inventory.warnings.items[0], "device_record_unavailable_or_changed") == 0);
        } else CHECK(inventory.warnings.count == 0);
        if (scenario == DEVICE_OOM) CHECK(strstr(error, "Memory allocation failed") != NULL);
        ds_inventory_free(&inventory);
    }
    printf("%u discovery boundary assertions passed; 7 scenarios.\n", checks);
    return 0;
}
