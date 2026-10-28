/** @file
 * @brief Decode bounded NVMe controller metadata and preserve independent method uncertainty.
 */
#include "disksan/protocol.h"
#include <string.h>

/** @brief Byte positions shared by the reviewed NVMe 1.3d, 1.4c and 2.0c Identify layouts. */
enum { ID_VID = 0, ID_CNTLID = 78, ID_VER = 80, ID_CNTRLTYPE = 111, ID_SANICAP = 328, ID_NN = 516 };
/** @brief Assemble little-endian bytes without alignment, aliasing or signed-shift assumptions. */
static uint32_t le32(const unsigned char *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8U) |
           ((uint32_t)bytes[2] << 16U) | ((uint32_t)bytes[3] << 24U);
}
/** @brief Read a little-endian identifier without borrowing native structure layout. */
static uint16_t le16(const unsigned char *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8U));
}
/** @brief Recognize only explicitly reviewed version values; future revisions stay unknown. */
static bool known_version(uint32_t version)
{
    return version == UINT32_C(0x00010300) || version == UINT32_C(0x00010400) ||
           version == UINT32_C(0x00020000);
}
/** @brief Reject unknown query kinds before any adapter may acquire a native handle. */
bool ds_protocol_query_allowed(ds_query_kind query)
{
    return query == DS_QUERY_NVME_IDENTIFY_CONTROLLER;
}
/** @brief Validate the exact reply envelope before parsing documented numeric fields. */
ds_nvme_decode_status ds_nvme_identify_decode(const unsigned char *bytes, size_t size, ds_nvme_controller *controller)
{
    memset(controller, 0, sizeof(*controller));
    if (!bytes || size != DS_NVME_IDENTIFY_SIZE) return DS_NVME_DECODE_LENGTH;
    uint32_t version = le32(bytes + ID_VER);
    if (!known_version(version)) return DS_NVME_DECODE_VERSION;
    uint16_t vendor = le16(bytes + ID_VID);
    uint32_t namespaces = le32(bytes + ID_NN), sanitize = le32(bytes + ID_SANICAP);
    bool legacy = version == UINT32_C(0x00010300);
    if (!vendor || vendor == UINT16_MAX || !namespaces || namespaces == UINT32_MAX ||
        bytes[ID_CNTRLTYPE] != (legacy ? 0U : 1U)) return DS_NVME_DECODE_FIELDS;
    if (legacy) {
        if (sanitize & ~UINT32_C(7)) return DS_NVME_DECODE_FIELDS;
    } else {
        uint32_t mode = sanitize >> 30U;
        if ((sanitize & UINT32_C(0x1ffffff8)) || mode == 3U ||
            (!(sanitize & 7U) && sanitize) || ((sanitize & 7U) && !mode)) return DS_NVME_DECODE_FIELDS;
    }
    controller->version = version;
    controller->namespace_limit = namespaces;
    controller->controller_id = le16(bytes + ID_CNTLID);
    controller->sanitize_support = (uint8_t)(sanitize & 7U);
    return DS_NVME_DECODE_OK;
}
/** @brief Poison invalid provider combinations before any result can be published. */
static int invalid_evidence(ds_assessment *result)
{
    result->failed = true; result->status = DS_INSPECTION_NOT_EVALUATED; result->finding_count = 0;
    return -1;
}
/** @brief Map only SANICAP support bits; Identify success cannot establish method access or scope. */
int ds_nvme_capabilities(const ds_nvme_controller *controller, ds_assessment *result)
{
    if (!known_version(controller->version) || !controller->namespace_limit ||
        controller->namespace_limit == UINT32_MAX || controller->sanitize_support > 7U ||
        result->failed || result->capability_count) return invalid_evidence(result);
    static const unsigned masks[] = {2U, 1U, 4U};
    static const char *const sources[] = {
        "nvme:identify_controller:SANICAP.BES", "nvme:identify_controller:SANICAP.CES",
        "nvme:identify_controller:SANICAP.OWS"
    };
    static const char *const unknowns[] = {
        "No audited ATA or SCSI protocol provider supplied method support evidence.",
        "Read-only Identify access does not establish access to any sanitization method.",
        "Controller support does not establish applicability to the selected namespace or physical media.",
        "A returned Identify payload does not establish bridge passthrough or the complete transport scope."
    };
    for (unsigned m = 0; m < DS_METHOD_COUNT; ++m) {
        for (unsigned d = 0; d < DS_CAP_DIMENSION_COUNT; ++d) {
            bool support = m < 3U && d == DS_CAP_SUPPORT;
            ds_cap_state state = support ? ((controller->sanitize_support & masks[m]) ? DS_CAP_SUPPORTED : DS_CAP_UNSUPPORTED) : DS_CAP_UNKNOWN;
            if (ds_assessment_capability(result, (ds_method)m, (ds_cap_dimension)d, state,
                    support ? sources[m] : "provider:audited_probe",
                    support ? "The validated controller SANICAP field advertises this method's support bit; no method was executed or authorized." : unknowns[d])) return -1;
        }
    }
    return 0;
}
