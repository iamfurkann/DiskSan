/** @file
 * @brief Exercise pure NVMe decode bounds, reserved fields and independent evidence without native calls.
 */
#include "disksan/protocol.h"
#include <stdlib.h>
#include <string.h>

static unsigned checks;
/** @brief Stop on the first failed contract without exposing raw protocol fixtures. */
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)
/** @brief Encode a synthetic little-endian field independently of host byte order. */
static void put32(unsigned char *bytes, uint32_t value)
{
    for (unsigned i = 0; i < 4U; ++i) bytes[i] = (unsigned char)(value >> (8U * i));
}
/** @brief Construct a synthetic I/O-controller reply without meaningful private text. */
static void fixture(unsigned char *bytes, uint32_t version, unsigned support)
{
    memset(bytes, 0, DS_NVME_IDENTIFY_SIZE);
    bytes[0] = 0x34; bytes[1] = 0x12; bytes[78] = 0x78; bytes[79] = 0x56;
    put32(bytes + 80, version); put32(bytes + 516, 0x12345678U);
    bytes[111] = version == 0x00010300U ? 0 : 1;
    put32(bytes + 328, support | (support && version != 0x00010300U ? 0x40000000U : 0U));
}
/** @brief Verify exact length checks and output clearing even with a previously valid result. */
static void lengths(void)
{
    unsigned char bytes[DS_NVME_IDENTIFY_SIZE + 1U]; ds_nvme_controller value;
    fixture(bytes, 0x00010400U, 7);
    for (size_t n = 0; n < DS_NVME_IDENTIFY_SIZE; ++n) {
        memset(&value, 0xa5, sizeof(value));
        CHECK(ds_nvme_identify_decode(bytes, n, &value) == DS_NVME_DECODE_LENGTH);
        CHECK(value.version == 0 && value.namespace_limit == 0 && value.controller_id == 0 && value.sanitize_support == 0);
    }
    CHECK(ds_nvme_identify_decode(bytes, sizeof(bytes), &value) == DS_NVME_DECODE_LENGTH);
    CHECK(ds_nvme_identify_decode(bytes, SIZE_MAX, &value) == DS_NVME_DECODE_LENGTH);
    CHECK(ds_nvme_identify_decode(NULL, DS_NVME_IDENTIFY_SIZE, &value) == DS_NVME_DECODE_LENGTH);
    /* A short pointer is safe when the length is rejected before reading fields. */
    unsigned char byte = 0;
    CHECK(ds_nvme_identify_decode(&byte, 1, &value) == DS_NVME_DECODE_LENGTH);
}
/** @brief Check every method bit independently and keep the other 21 dimensions unknown. */
static void evidence(void)
{
    unsigned char storage[DS_NVME_IDENTIFY_SIZE + 1U]; unsigned char *bytes = storage + 1;
    const uint32_t versions[] = {0x00010300U, 0x00010400U, 0x00020000U};
    const unsigned masks[] = {2U, 1U, 4U};
    for (size_t v = 0; v < sizeof(versions) / sizeof(versions[0]); ++v) {
        for (unsigned bits = 0; bits < 8U; ++bits) {
            fixture(bytes, versions[v], bits); ds_nvme_controller controller;
            CHECK(ds_nvme_identify_decode(bytes, DS_NVME_IDENTIFY_SIZE, &controller) == DS_NVME_DECODE_OK);
            CHECK(controller.version == versions[v] && controller.controller_id == 0x5678U);
            CHECK(controller.namespace_limit == 0x12345678U && controller.sanitize_support == bits);
            ds_assessment a; ds_assessment_init(&a);
            CHECK(ds_nvme_capabilities(&controller, &a) == 0 && a.capability_count == 24U);
            for (size_t i = 0; i < a.capability_count; ++i) {
                ds_cap_evidence *e = &a.capabilities[i];
                bool supported_field = e->method < DS_METHOD_ATA_SECURITY && e->dimension == DS_CAP_SUPPORT;
                CHECK(e->state == (supported_field ? ((bits & masks[e->method]) ? DS_CAP_SUPPORTED : DS_CAP_UNSUPPORTED) : DS_CAP_UNKNOWN));
                CHECK(strstr(e->source, supported_field ? "SANICAP." : "audited_probe") != NULL);
            }
            CHECK(ds_assessment_evaluate(&a) == 0 && a.status != DS_INSPECTION_NO_BLOCKERS);
            CHECK(ds_nvme_capabilities(&controller, &a) == -1 && a.failed);
            CHECK(ds_assessment_evaluate(&a) == -1);
        }
    }
}
/** @brief Reject unreviewed versions, impossible headers, reserved fields and contradictory sanitize attributes. */
static void malformed(void)
{
    unsigned char bytes[DS_NVME_IDENTIFY_SIZE]; ds_nvme_controller controller;
    const uint32_t invalid_versions[] = {0, 0x00010200U, 0x00010301U, 0x00010500U, 0x00020100U, 0x00030000U, UINT32_MAX};
    for (size_t i = 0; i < sizeof(invalid_versions) / sizeof(invalid_versions[0]); ++i) {
        fixture(bytes, invalid_versions[i], 7);
        CHECK(ds_nvme_identify_decode(bytes, sizeof(bytes), &controller) == DS_NVME_DECODE_VERSION);
        CHECK(controller.version == 0 && controller.sanitize_support == 0);
    }
    for (unsigned bit = 3; bit < 32U; ++bit) {
        fixture(bytes, 0x00010300U, 7); put32(bytes + 328, 7U | (UINT32_C(1) << bit));
        CHECK(ds_nvme_identify_decode(bytes, sizeof(bytes), &controller) == DS_NVME_DECODE_FIELDS);
    }
    for (unsigned bit = 3; bit < 29U; ++bit) {
        fixture(bytes, 0x00010400U, 7); put32(bytes + 328, 0x40000007U | (UINT32_C(1) << bit));
        CHECK(ds_nvme_identify_decode(bytes, sizeof(bytes), &controller) == DS_NVME_DECODE_FIELDS);
    }
    const uint32_t bad_caps[] = {7U, 0xc0000007U, 0x20000000U, 0x40000000U, 0x80000000U};
    for (size_t i = 0; i < sizeof(bad_caps) / sizeof(bad_caps[0]); ++i) {
        fixture(bytes, 0x00020000U, 7); put32(bytes + 328, bad_caps[i]);
        CHECK(ds_nvme_identify_decode(bytes, sizeof(bytes), &controller) == DS_NVME_DECODE_FIELDS);
    }
    for (unsigned type = 0; type < 256U; ++type) {
        fixture(bytes, 0x00010400U, 1); bytes[111] = (unsigned char)type;
        CHECK(ds_nvme_identify_decode(bytes, sizeof(bytes), &controller) == (type == 1U ? DS_NVME_DECODE_OK : DS_NVME_DECODE_FIELDS));
    }
    fixture(bytes, 0x00010400U, 7); bytes[0] = bytes[1] = 0;
    CHECK(ds_nvme_identify_decode(bytes, sizeof(bytes), &controller) == DS_NVME_DECODE_FIELDS);
    bytes[0] = bytes[1] = 0xff;
    CHECK(ds_nvme_identify_decode(bytes, sizeof(bytes), &controller) == DS_NVME_DECODE_FIELDS);
    fixture(bytes, 0x00010400U, 7); put32(bytes + 516, 0);
    CHECK(ds_nvme_identify_decode(bytes, sizeof(bytes), &controller) == DS_NVME_DECODE_FIELDS);
    put32(bytes + 516, UINT32_MAX);
    CHECK(ds_nvme_identify_decode(bytes, sizeof(bytes), &controller) == DS_NVME_DECODE_FIELDS);
    fixture(bytes, 0x00010400U, 7); put32(bytes + 328, 0xa0000007U);
    CHECK(ds_nvme_identify_decode(bytes, sizeof(bytes), &controller) == DS_NVME_DECODE_OK);
    ds_assessment a; ds_assessment_init(&a); controller.sanitize_support = 8;
    CHECK(ds_nvme_capabilities(&controller, &a) == -1 && a.failed);
}
/** @brief Run repeatable arbitrary-byte cases and clear stale positive evidence after every failure. */
static void randomized(void)
{
    unsigned char bytes[DS_NVME_IDENTIFY_SIZE]; ds_nvme_controller value; uint32_t seed = 0x0d15c5a1U;
    for (unsigned trial = 0; trial < 1024U; ++trial) {
        for (size_t i = 0; i < sizeof(bytes); ++i) {
            seed ^= seed << 13U; seed ^= seed >> 17U; seed ^= seed << 5U; bytes[i] = (unsigned char)seed;
        }
        /* Force a known version periodically to reach field validation as well. */
        if (trial & 1U) put32(bytes + 80, 0x00010400U);
        ds_nvme_decode_status status = ds_nvme_identify_decode(bytes, sizeof(bytes), &value);
        CHECK(status != DS_NVME_DECODE_OK);
        CHECK(value.version == 0 && value.namespace_limit == 0 && value.sanitize_support == 0);
    }
}
/** @brief Run bounded pure metadata tests, including a closed request-kind allowlist. */
int main(void)
{
    for (unsigned query = 0; query < 256U; ++query)
        CHECK(ds_protocol_query_allowed((ds_query_kind)query) == (query == 1U));
    CHECK(!ds_protocol_query_allowed((ds_query_kind)-1));
    lengths(); evidence(); malformed(); randomized();
    printf("Protocol decoder and evidence: %u assertions passed.\n", checks);
    return 0;
}
