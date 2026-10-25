/**
 * @file
 * @brief Parse the read-only CLI and dispatch discovery and output.
 */
#include "disksan/discovery.h"
#include "disksan/version.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

/** @brief Write CLI usage and return the requested status unless flushing the output fails. */
static int usage(FILE *out, int code)
{
    fputs("DiskSan - read-only device discovery\n"
          "Usage:\n"
          "  disksan --help\n"
          "  disksan --version\n"
          "  disksan list [--all] [--json]\n"
          "  disksan info [--json] [--] <block-device> [--json]\n"
          "Read-only inventory for Linux, macOS and Windows. No erase operations exist.\n"
          "Default list omits partitions, volumes and snapshots; --all includes them.\n"
          "Examples: disksan list --json; disksan info /dev/sda --json\n"
          "Exit: 0 success, 1 system, 2 usage, 3 invalid target, 4 permission.\n", out);
    return fflush(out) == 0 ? code : 1;
}

/** @brief Validate UTF-8 arguments, discover a snapshot, render it, and release its memory. */
static int run_main(int argc, char **argv)
{
#ifdef SIGPIPE
    (void)signal(SIGPIPE, SIG_IGN);
#endif
    if (argc == 2 && strcmp(argv[1], "--help") == 0) return usage(stdout, 0);
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        printf("DiskSan v%s\n", DISKSAN_VERSION);
        return fflush(stdout) == 0 ? 0 : 1;
    }
    if (argc < 2) return usage(stderr, 2);
    bool list = strcmp(argv[1], "list") == 0;
    bool info = strcmp(argv[1], "info") == 0;
    if (!list && !info) return usage(stderr, 2);
    bool json = false, all = false, end_options = false;
    const char *selector = NULL;
    for (int i = 2; i < argc; ++i) {
        if (!end_options && strcmp(argv[i], "--") == 0) { end_options = true; continue; }
        if (!end_options && strcmp(argv[i], "--json") == 0 && !json) json = true;
        else if (!end_options && strcmp(argv[i], "--all") == 0 && list && !all) all = true;
        else if (info && !selector && (end_options || argv[i][0] != '-')) selector = argv[i];
        else return usage(stderr, 2);
    }
    if (info && (!selector || !*selector)) return usage(stderr, 2);
    ds_inventory inventory = {0};
    char error[512] = {0};
    int result = ds_discover(&inventory, selector, error, sizeof(error));
    if (result != 0) {
        fputs("DiskSan: ", stderr);
        ds_terminal_string(stderr, error, sizeof(error));
        fputc('\n', stderr);
    } else {
        ds_inventory_sort(&inventory);
        result = ds_output(stdout, &inventory, json, all || info, info);
        if (result != 0) fputs("DiskSan: output failed.\n", stderr);
    }
    ds_inventory_free(&inventory);
    return result;
}

#ifdef _WIN32
/** @brief Convert Windows UTF-16 arguments to UTF-8 and free all temporary argument buffers. */
int wmain(int argc, wchar_t **wide_argv)
{
    char **argv = calloc((size_t)argc + 1U, sizeof(*argv));
    if (!argv) return 1;
    int result = 1;
    for (int i = 0; i < argc; ++i) {
        int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide_argv[i], -1, NULL, 0, NULL, NULL);
        if (bytes <= 0) goto done;
        argv[i] = malloc((size_t)bytes);
        if (!argv[i] || !WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide_argv[i], -1, argv[i], bytes, NULL, NULL)) goto done;
    }
    result = run_main(argc, argv);
done:
    for (int i = 0; i < argc; ++i) free(argv[i]);
    free(argv);
    return result;
}
#else
/** @brief Dispatch the native UTF-8 command line and return the documented CLI exit status. */
int main(int argc, char **argv) { return run_main(argc, argv); }
#endif
