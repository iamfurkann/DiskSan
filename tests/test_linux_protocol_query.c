/** @file
 * @brief Capture the actual Linux ioctl request and forbid arbitrary opcodes, lengths, metadata and retries without real devices.
 */
#include "../src/platform/linux/protocol_query.h"
#include <errno.h>
#include <linux/nvme_ioctl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

static unsigned checks, calls;
static int completion, native_error;
static unsigned char *expected_response;
/** @brief Stop at a violated synthetic boundary without invoking the real kernel operation. */
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)
/** @brief Independently assert every command byte, exact ioctl/descriptor and initialized reply before injecting a completion. */
int __wrap_ioctl(int fd, unsigned long request, ...)
{
    ++calls; CHECK(calls == 1U && fd == 43);
    CHECK(request == NVME_IOCTL_ADMIN_CMD);
    CHECK(request != NVME_IOCTL_ADMIN64_CMD && request != NVME_IOCTL_IO_CMD &&
          request != NVME_IOCTL_SUBMIT_IO && request != NVME_IOCTL_RESET &&
          request != NVME_IOCTL_SUBSYS_RESET && request != NVME_IOCTL_RESCAN);
    va_list args; va_start(args, request);
    const struct nvme_passthru_cmd *actual = va_arg(args, struct nvme_passthru_cmd *);
    va_end(args);
    struct nvme_passthru_cmd expected; memset(&expected, 0, sizeof(expected));
    expected.opcode = 6; expected.cdw10 = 1; expected.addr = (uint64_t)(uintptr_t)expected_response;
    expected.data_len = 4096; expected.timeout_ms = 5000;
    CHECK(sizeof(expected) == 72U && !memcmp(actual, &expected, sizeof(expected)));
    CHECK(actual->opcode != 0x80U && actual->opcode != 0x84U && actual->opcode != 0x09U);
    for (size_t i = 0; i < 4096U; ++i) CHECK(expected_response[i] == 0);
    memset(expected_response, 0xa5, 4096);
    errno = native_error;
    return completion;
}
/** @brief Check all request identifiers and exact completion/error propagation without fallbacks. */
int main(void)
{
    unsigned char response[DS_NVME_IDENTIFY_SIZE]; expected_response = response;
    for (unsigned kind = 0; kind < 256U; ++kind) {
        calls = 0; completion = 0; native_error = 0; memset(response, 0x5a, sizeof(response));
        int rc = ds_linux_nvme_identify(43, (ds_query_kind)kind, response);
        CHECK(rc == (kind == 1U ? 0 : -1));
        CHECK(calls == (kind == 1U ? 1U : 0U));
        if (kind != 1U) { CHECK(errno == EINVAL && response[0] == 0x5a && response[4095] == 0x5a); }
    }
    calls = 0;
    CHECK(ds_linux_nvme_identify(43, (ds_query_kind)-1, response) == -1 && calls == 0U);
    CHECK(ds_linux_nvme_identify(-1, DS_QUERY_NVME_IDENTIFY_CONTROLLER, response) == -1 && calls == 0U);
    CHECK(ds_linux_nvme_identify(43, DS_QUERY_NVME_IDENTIFY_CONTROLLER, NULL) == -1 && calls == 0U);
    const int errors[] = {EACCES, EPERM, EIO, ENOTTY, ETIMEDOUT, EINTR, ENODEV};
    for (size_t i = 0; i < sizeof(errors) / sizeof(errors[0]); ++i) {
        calls = 0; completion = -1; native_error = errors[i];
        CHECK(ds_linux_nvme_identify(43, DS_QUERY_NVME_IDENTIFY_CONTROLLER, response) == -1);
        CHECK(errno == errors[i] && calls == 1U);
    }
    calls = 0; completion = 0x4002; native_error = 0;
    CHECK(ds_linux_nvme_identify(43, DS_QUERY_NVME_IDENTIFY_CONTROLLER, response) == 0x4002 && calls == 1U);
    printf("Linux fixed protocol query: %u assertions passed.\n", checks);
    return 0;
}
