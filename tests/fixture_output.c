/**
 * @file
 * @brief Generate synthetic output fixtures without accessing real devices.
 */
#include "disksan/device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** @brief Terminate fixture construction when a metadata operation fails. */
#define MUST(expr) do { if ((expr) < 0) { ds_inventory_free(&inventory); ds_device_free(&d); return 1; } } while (0)
/** @brief Construct the requested hostile or boundary fixture and return the serialization status. */
int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "hostile";
    ds_inventory inventory = {0};
    ds_device d;
    ds_device_init(&d);
    if (strcmp(mode, "empty") != 0) {
        d.major_number = 8;
        d.minor_number = 0;
        ds_facts facts = {.disk=true};
        if (strcmp(mode, "virtual") == 0) facts.virtio = true;
        if (strcmp(mode, "partition") == 0) facts.partition = true;
        MUST(ds_text_offer(&d, &d.path, "/dev/example\n\033[31m", "fixture"));
        MUST(ds_text_offer(&d, &d.model, "quote\" slash\\ newline\n tab\t ESC\033 DEL\177 C1:\xc2\x9b UTF-8: Unicode: \xc3\xa9 \xf0\x9f\x98\x80 invalid:\xff\xc0\xaf\xed\xa0\x80\xf4\x90\x80\x80 truncated:\xe2\x82", "fixture"));
        if (strcmp(mode, "long") == 0) {
            char long_model[DS_TEXT_LIMIT + 1U];
            memset(long_model, 'Z', DS_TEXT_LIMIT);
            long_model[DS_TEXT_LIMIT] = '\0';
            free(d.model.value); d.model.value = NULL; d.model.source = NULL;
            MUST(ds_text_offer(&d, &d.model, long_model, "fixture"));
        }
        if (strcmp(mode, "bytes") == 0) {
            char input[DS_TEXT_LIMIT + 1U];
            size_t len = fread(input, 1, DS_TEXT_LIMIT, stdin);
            input[len] = '\0';
            free(d.model.value); d.model.value = NULL; d.model.source = NULL;
            MUST(ds_text_offer(&d, &d.model, input, "fixture"));
        }
        if (strcmp(mode, "overflow") == 0) {
            uint64_t n;
            if (ds_sectors_to_bytes("36028797018963968", &n)) return 1;
            MUST(ds_warn(&d, "invalid_numeric_metadata"));
        } else if (strcmp(mode, "zero") == 0) {
            MUST(ds_number_offer(&d, &d.size_bytes, 0, "fixture"));
        } else if (strcmp(mode, "unknown") != 0) {
            MUST(ds_number_offer(&d, &d.size_bytes, UINT64_MAX, "fixture"));
        }
        if (strcmp(mode, "conflict") == 0) {
            MUST(ds_number_offer(&d, &d.size_bytes, 4096, "alternative"));
            MUST(ds_text_offer(&d, &d.model, "alternative model", "alternative"));
        }
        MUST(ds_classify(&d, &facts));
        MUST(ds_inventory_add(&inventory, &d));
    }
    ds_inventory_sort(&inventory);
    bool text = argc > 2 && strcmp(argv[2], "text") == 0;
    bool all = argc > 3 && strcmp(argv[3], "all") == 0;
    bool detail = argc > 4 && strcmp(argv[4], "detail") == 0;
    int rc = ds_output(stdout, &inventory, !text, all, detail);
    ds_inventory_free(&inventory);
    ds_device_free(&d);
    return rc;
}
