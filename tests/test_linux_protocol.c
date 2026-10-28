/** @file
 * @brief Exercise the real Linux protocol mapping, request and descriptor lifecycle with entirely synthetic native boundaries.
 */
#include "disksan/protocol.h"
#include <errno.h>
#include <fcntl.h>
#include <libudev.h>
#include <linux/nvme_ioctl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

static unsigned checks, scenarios, maps, opens, queries, closes, phase;
static bool context_live, namespace_live, descriptor_live;
static const char *fault;
static int native_error;
static char oversized[DS_TEXT_LIMIT + 2U];
static const char ns_path[] = "/sys/devices/pci-fixture/nvme/controller/namespace";
static const char ctrl_path[] = "/sys/devices/pci-fixture/nvme/controller";
static const char ns_node[] = "/dev/fixture-namespace";
static const char ctrl_node[] = "/dev/fixture-controller";
/** @brief Supply opaque libudev objects without accessing the real library or sysfs. */
struct udev { unsigned id; };
/** @brief Distinguish synthetic namespace, controller and PCI ancestry. */
struct udev_device { unsigned kind; };
static struct udev context;
static struct udev_device ns = {0}, ctrl = {1}, pci = {2};
/** @brief Assert exact native calls without exposing machine identifiers. */
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%s phase %u line %d: %s\n", fault, phase, __LINE__, #x); exit(1); } } while (0)
/** @brief Activate one specific failure only at its selected fresh mapping phase. */
static bool failing(const char *name) { return !strcmp(fault, name) && (phase == 0U || maps == phase); }
/** @brief Count fresh contexts, rejecting simultaneous retained snapshots. */
struct udev *udev_new(void)
{
    ++maps; CHECK(!context_live && !namespace_live);
    if (failing("allocation")) { errno = ENOMEM; return NULL; }
    context_live = true; return &context;
}
/** @brief Verify that borrowed parent objects never outlive the owned namespace context. */
struct udev *udev_unref(struct udev *value)
{
    CHECK(value == &context && context_live && !namespace_live); context_live = false; return NULL;
}
/** @brief Resolve exactly the selected block number; never create a controller from a guessed name. */
struct udev_device *udev_device_new_from_devnum(struct udev *value, char type, dev_t number)
{
    CHECK(value == &context && context_live && type == 'b' && number == makedev(259U, 7U));
    if (failing("device-allocation") || failing("gone")) {
        errno = failing("device-allocation") ? ENOMEM : ENODEV; return NULL;
    }
    CHECK(!namespace_live); namespace_live = true; return &ns;
}
/** @brief Close exactly the owned namespace object, not borrowed ancestors. */
struct udev_device *udev_device_unref(struct udev_device *value)
{
    CHECK(value == &ns && namespace_live); namespace_live = false; return NULL;
}
/** @brief Model direct ancestry and missing/aggregate parent faults. */
struct udev_device *udev_device_get_parent(struct udev_device *value)
{
    CHECK(namespace_live && context_live);
    if (value == &ns) return failing("no-parent") ? NULL : &ctrl;
    CHECK(value == &ctrl); return failing("no-pci") ? NULL : &pci;
}
/** @brief Supply kernel subsystem names independently of discovery classification. */
const char *udev_device_get_subsystem(struct udev_device *value)
{
    CHECK(namespace_live);
    if (value == &ns) return failing("namespace-subsystem") ? "other" : "block";
    if (value == &ctrl) return failing("aggregate") ? "nvme-subsystem" : "nvme";
    CHECK(value == &pci); return failing("pci-subsystem") ? "usb" : "pci";
}
/** @brief Refuse partitions even if the selected snapshot was falsely classified as a namespace. */
const char *udev_device_get_devtype(struct udev_device *value)
{
    CHECK(value == &ns && namespace_live); return failing("partition") ? "partition" : "disk";
}
/** @brief Check the actual PCI driver instead of model-name heuristics. */
const char *udev_device_get_driver(struct udev_device *value)
{
    CHECK(value == &pci && namespace_live); return failing("driver") ? "other" : "nvme";
}
/** @brief Preserve namespace and controller sysfs identities or simulate replacement. */
const char *udev_device_get_syspath(struct udev_device *value)
{
    CHECK(namespace_live);
    if (value == &ns) return failing("namespace-path") ? "/sys/devices/other" : ns_path;
    CHECK(value == &ctrl); return failing("controller-path") ? "/sys/devices/other" : ctrl_path;
}
/** @brief Return deliberately nonnumeric node names to prove no namespace-name derivation is used. */
const char *udev_device_get_devnode(struct udev_device *value)
{
    CHECK(namespace_live);
    if (value == &ns) return failing("namespace-node") ? "/dev/other" : ns_node;
    CHECK(value == &ctrl);
    if (failing("missing-node")) return NULL;
    if (failing("long-node")) return oversized;
    return failing("relative-node") ? "relative" : ctrl_node;
}
/** @brief Supply separate block and character device numbers with reuse fault injection. */
dev_t udev_device_get_devnum(struct udev_device *value)
{
    CHECK(namespace_live);
    if (value == &ns) return makedev(259U, failing("namespace-number") ? 8U : 7U);
    CHECK(value == &ctrl);
    return failing("missing-number") ? 0 : makedev(240U, failing("controller-number") ? 2U : 1U);
}
/** @brief Allow only five fixed cached attributes, never nuse or any protocol-triggering sysfs field. */
const char *udev_device_get_sysattr_value(struct udev_device *value, const char *name)
{
    CHECK(namespace_live);
    if (value == &ns) {
        if (!strcmp(name, "diskseq")) {
            if (failing("missing-generation")) return NULL;
            if (failing("long-generation")) return oversized;
            if (failing("zero-generation")) return "0";
            if (failing("bad-generation")) return "18446744073709551616";
            return failing("changed-generation") ? "42" : "41";
        }
        CHECK(!strcmp(name, "nsid"));
        if (failing("missing-nsid")) return NULL;
        if (failing("zero-nsid")) return "0";
        if (failing("broadcast-nsid")) return "4294967295";
        if (failing("bad-nsid")) return "-1";
        if (failing("nsid-outside-response")) return "5";
        return failing("changed-nsid") ? "2" : "1";
    }
    CHECK(value == &ctrl);
    if (!strcmp(name, "transport")) return failing("transport") ? "tcp" : "pcie";
    if (!strcmp(name, "state")) return failing("state") ? "resetting" : "live";
    CHECK(!strcmp(name, "cntlid"));
    if (failing("missing-cntlid")) return NULL;
    if (failing("bad-cntlid")) return "65535";
    return failing("changed-cntlid") ? "8" : "7";
}
/** @brief Initialize synthetic node metadata without inspecting any real file or device. */
static void node_stat(struct stat *st, bool controller)
{
    memset(st, 0, sizeof(*st)); st->st_mode = controller ? S_IFCHR : S_IFBLK;
    st->st_rdev = controller ? makedev(240U, 1U) : makedev(259U, 7U);
    st->st_dev = 8; st->st_ino = controller ? 100U : 101U;
}
/** @brief Exercise type, number, inode and path failures without traversing real nodes or symlinks. */
int __wrap_lstat(const char *path, struct stat *st)
{
    bool controller = !strcmp(path, ctrl_node);
    CHECK(controller || !strcmp(path, ns_node)); node_stat(st, controller);
    if (failing("stat-failure")) { errno = ENOENT; return -1; }
    if (controller && failing("symlink")) st->st_mode = S_IFLNK;
    if (!controller && failing("ordinary-file")) st->st_mode = S_IFREG;
    if (controller && failing("wrong-number")) st->st_rdev = makedev(240U, 99U);
    if (failing("inode-change")) ++st->st_ino;
    if (failing("filesystem-change")) ++st->st_dev;
    return 0;
}
/** @brief Assert the complete read-only open flags and exactly one controller path; return only a fake descriptor. */
int __wrap_open(const char *path, int flags, ...)
{
    ++opens; CHECK(opens == 1U && maps == 1U && !descriptor_live);
    CHECK(!strcmp(path, ctrl_node));
    CHECK(flags == (O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    CHECK((flags & O_ACCMODE) == O_RDONLY && !(flags & (O_CREAT | O_TRUNC | O_APPEND)));
    if (failing("open-failure")) { errno = native_error; return -1; }
    descriptor_live = true; return 43;
}
/** @brief Match the opened descriptor or inject a post-open substitution/error. */
int __wrap_fstat(int fd, struct stat *st)
{
    CHECK(fd == 43 && descriptor_live && maps == 1U && queries == 0U); node_stat(st, true);
    if (failing("fstat-failure")) { errno = EIO; return -1; }
    if (failing("fstat-type")) st->st_mode = S_IFBLK;
    if (failing("fstat-number")) st->st_rdev = makedev(240U, 99U);
    if (failing("fstat-inode")) ++st->st_ino;
    if (failing("fstat-filesystem")) ++st->st_dev;
    return 0;
}
/** @brief Capture the actual fixed command independently and return bounded synthetic controller metadata. */
int __wrap_ioctl(int fd, unsigned long request, ...)
{
    ++queries; CHECK(fd == 43 && descriptor_live && maps == 2U && queries == 1U);
    CHECK(request == NVME_IOCTL_ADMIN_CMD && request != NVME_IOCTL_ADMIN64_CMD && request != NVME_IOCTL_IO_CMD);
    va_list args; va_start(args, request); struct nvme_passthru_cmd *cmd = va_arg(args, struct nvme_passthru_cmd *); va_end(args);
    struct nvme_passthru_cmd expected; memset(&expected, 0, sizeof(expected));
    expected.opcode = 6; expected.cdw10 = 1; expected.addr = cmd->addr; expected.data_len = 4096; expected.timeout_ms = 5000;
    CHECK(cmd->addr != 0 && !memcmp(cmd, &expected, sizeof(expected)));
    unsigned char *reply = (unsigned char *)(uintptr_t)cmd->addr;
    for (size_t i = 0; i < 4096U; ++i) CHECK(reply[i] == 0U);
    if (failing("ioctl-failure")) { errno = native_error; return -1; }
    if (failing("nvme-status")) return 0x4002;
    if (failing("empty-response")) return 0;
    reply[0] = 1; reply[78] = 7; reply[82] = 1; reply[81] = 4; reply[111] = 1;
    reply[328] = 3; reply[331] = 0x40; reply[516] = 4;
    if (failing("bad-response")) reply[328] |= 8;
    if (failing("unknown-version")) reply[81] = 5;
    if (failing("response-cntlid")) reply[78] = 8;
    if (failing("response-nsid")) reply[516] = 0;
    memcpy(reply + 4, "PRIVATE-REPLY", 13); return 0;
}
/** @brief Require exactly one close on every opened path, including EINTR without retry. */
int __wrap_close(int fd)
{
    ++closes; CHECK(fd == 43 && descriptor_live && closes == 1U); descriptor_live = false;
    if (failing("close-failure")) { errno = native_error; return -1; }
    return 0;
}
/** @brief Build the selected snapshot separately from the native wrappers. */
static void selected(ds_device *device)
{
    ds_device_init(device); device->platform = "linux"; device->role = "namespace";
    device->transport = "nvme"; device->protocol = "nvme"; device->has_devnum = true;
    device->major_number = 259; device->minor_number = 7;
    CHECK(ds_text_offer(device, &device->path, ns_node, "fixture") == 0);
    CHECK(ds_text_offer(device, &device->sysfs_path, ns_path, "fixture") == 0);
    CHECK(ds_text_offer(device, &device->controller, ctrl_path, "fixture") == 0);
    char identity[DS_TEXT_LIMIT + 1U];
    CHECK(snprintf(identity, sizeof(identity), "%s#diskseq=41", ns_path) > 0);
    CHECK(ds_text_offer(device, &device->device_id, identity, "sysfs:syspath+diskseq") == 0);
}
/** @brief Reset counters only after every native object and descriptor was released. */
static void reset(const char *name, unsigned at, int error)
{
    CHECK(!context_live && !namespace_live && !descriptor_live); ++scenarios;
    fault = name; phase = at; native_error = error; maps = opens = queries = closes = 0;
}
/** @brief Assert receipt truth and zero retained response facts for every failure outcome. */
static void run(const ds_device *device, int code, ds_probe_status status, unsigned expected_queries)
{
    ds_nvme_controller result; memset(&result, 0xa5, sizeof(result));
    ds_probe_report report = {0}; char error[128] = {0};
    CHECK(ds_native_protocol_probe(device, DS_QUERY_NVME_IDENTIFY_CONTROLLER, &result, &report, error, sizeof(error)) == code);
    CHECK(!context_live && !namespace_live && !descriptor_live && closes <= opens);
    CHECK(queries == expected_queries && report.performed == (queries == 1U));
    if (code) CHECK(error[0] && !result.version && !result.namespace_limit && !result.controller_id && !result.sanitize_support);
    else {
        CHECK(error[0] == '\0' && ds_probe_report_valid(&report) && report.status == status);
        if (!strcmp(fault, "open-failure") || !strcmp(fault, "ioctl-failure")) {
            if (native_error == EACCES || native_error == EPERM) CHECK(report.reason == DS_PROBE_PERMISSION);
            else if (native_error == ENOTTY || native_error == ENOSYS || native_error == EOPNOTSUPP)
                CHECK(report.reason == DS_PROBE_NATIVE_API);
            else CHECK(report.reason == DS_PROBE_QUERY_FAILED);
        }
        if (status == DS_PROBE_COMPLETE) CHECK(result.version == 0x10400U && result.sanitize_support == 3U && result.controller_id == 7U);
        else CHECK(!result.version && !result.namespace_limit && !result.controller_id && !result.sanitize_support);
    }
}
/** @brief Normalize actual adapter outcomes into independent dimensions, never method access from query success. */
static void evidence(const ds_device *device)
{
    const int errors[] = {0, EACCES, EIO};
    for (size_t i = 0; i < sizeof(errors) / sizeof(errors[0]); ++i) {
        reset(errors[i] ? "ioctl-failure" : "complete", 0, errors[i]);
        ds_assessment result; ds_assessment_init(&result); char error[128] = {0};
        CHECK(ds_collect_probed_capabilities(device, &result, error, sizeof(error)) == 0);
        CHECK(result.capability_count == 24U && !result.failed && queries == 1U && closes == 1U);
        for (size_t j = 0; j < result.capability_count; ++j) {
            const ds_cap_evidence *entry = &result.capabilities[j];
            if (entry->method < DS_METHOD_ATA_SECURITY && entry->dimension == DS_CAP_SUPPORT && !errors[i])
                CHECK(entry->state == (entry->method == DS_METHOD_NVME_OVERWRITE ? DS_CAP_UNSUPPORTED : DS_CAP_SUPPORTED));
            else if (entry->method < DS_METHOD_ATA_SECURITY && entry->dimension == DS_CAP_ACCESS && errors[i] == EACCES)
                CHECK(entry->state == DS_CAP_UNAVAILABLE);
            else CHECK(entry->state == DS_CAP_UNKNOWN);
        }
    }
}
/** @brief Reject all unknown request IDs and unsupported selected classifications before any udev or device call. */
static void gating(ds_device *device)
{
    for (unsigned kind = 0; kind < 256U; ++kind) if (kind != 1U) {
        reset("unknown-query", 0, 0); ds_nvme_controller result; ds_probe_report report; char error[128];
        CHECK(ds_native_protocol_probe(device, (ds_query_kind)kind, &result, &report, error, sizeof(error)) == 1);
        CHECK(maps == 0U && opens == 0U && queries == 0U && closes == 0U);
    }
    const char **fields[] = {&device->platform, &device->role, &device->protocol, &device->transport};
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i) {
        const char *saved = *fields[i]; *fields[i] = "unknown";
        reset("classification", 0, 0); run(device, 0, DS_PROBE_UNAVAILABLE, 0);
        CHECK(maps == 0U && opens == 0U); *fields[i] = saved;
    }
    CHECK(ds_strings_add(&device->holders, "synthetic-layer") == 0);
    reset("holder", 0, 0); run(device, 0, DS_PROBE_UNAVAILABLE, 0); CHECK(maps == 0U);
    ds_strings_free(&device->holders);
    CHECK(ds_strings_add(&device->slaves, "synthetic-layer") == 0);
    reset("slave", 0, 0); run(device, 0, DS_PROBE_UNAVAILABLE, 0); CHECK(maps == 0U);
    ds_strings_free(&device->slaves);
}
/** @brief Exercise unavailable initial mappings and fatal later loss without opening or querying a replacement. */
static void mapping_faults(const ds_device *device)
{
    const char *const unknown[] = {"no-parent", "no-pci", "aggregate", "pci-subsystem", "driver", "transport", "state",
        "missing-generation", "long-generation", "zero-generation", "bad-generation", "missing-nsid", "zero-nsid",
        "broadcast-nsid", "bad-nsid", "missing-cntlid", "bad-cntlid", "missing-node", "long-node", "relative-node", "missing-number"};
    const char *const changed[] = {"gone", "namespace-subsystem", "partition", "namespace-number", "namespace-path", "namespace-node",
        "controller-path", "changed-generation", "controller-number", "stat-failure", "symlink", "ordinary-file", "wrong-number"};
    for (unsigned at = 1; at <= 3U; ++at) {
        for (size_t i = 0; i < sizeof(unknown) / sizeof(unknown[0]); ++i) {
            reset(unknown[i], at, 0); run(device, at == 1U ? 0 : 3, DS_PROBE_UNAVAILABLE, at == 3U ? 1U : 0U);
            CHECK(opens == (at == 1U ? 0U : 1U) && closes == opens);
        }
        for (size_t i = 0; i < sizeof(changed) / sizeof(changed[0]); ++i) {
            reset(changed[i], at, 0); run(device, 3, DS_PROBE_UNAVAILABLE, at == 3U ? 1U : 0U);
            CHECK(opens == (at == 1U ? 0U : 1U) && closes == opens);
        }
        for (unsigned alloc = 0; alloc < 2U; ++alloc) {
            reset(alloc ? "device-allocation" : "allocation", at, 0);
            run(device, 1, DS_PROBE_UNAVAILABLE, at == 3U ? 1U : 0U);
            CHECK(closes == opens);
        }
    }
    const char *const replacements[] = {"inode-change", "filesystem-change", "changed-nsid", "changed-cntlid"};
    for (unsigned at = 2; at <= 3U; ++at) for (size_t i = 0; i < sizeof(replacements) / sizeof(replacements[0]); ++i) {
        reset(replacements[i], at, 0); run(device, 3, DS_PROBE_UNAVAILABLE, at == 3U ? 1U : 0U); CHECK(closes == 1U);
    }
}
/** @brief Sweep native syscall, completion, response and cleanup errors with no writable or alias fallback. */
static void native_faults(const ds_device *device)
{
    const int errors[] = {EACCES, EPERM, EIO, ENOTTY, ENOSYS, EOPNOTSUPP, ETIMEDOUT, EINTR, ENODEV, ENXIO, ENOENT};
    for (size_t i = 0; i < sizeof(errors) / sizeof(errors[0]); ++i) {
        bool lost = errors[i] == ENODEV || errors[i] == ENXIO || errors[i] == ENOENT;
        reset("open-failure", 0, errors[i]); run(device, lost ? 3 : 0, DS_PROBE_UNAVAILABLE, 0); CHECK(opens == 1U && closes == 0U);
        reset("ioctl-failure", 0, errors[i]); run(device, lost ? 3 : 0, DS_PROBE_UNAVAILABLE, 1); CHECK(opens == 1U && closes == 1U && maps == 3U);
    }
    const char *const fstats[] = {"fstat-failure", "fstat-type", "fstat-number", "fstat-inode", "fstat-filesystem"};
    for (size_t i = 0; i < sizeof(fstats) / sizeof(fstats[0]); ++i) {
        reset(fstats[i], 0, 0); run(device, i == 0U ? 1 : 3, DS_PROBE_UNAVAILABLE, 0); CHECK(closes == 1U && maps == 1U);
    }
    reset("nvme-status", 0, 0); run(device, 0, DS_PROBE_UNAVAILABLE, 1);
    reset("empty-response", 0, 0); run(device, 0, DS_PROBE_UNAVAILABLE, 1);
    reset("bad-response", 0, 0); run(device, 0, DS_PROBE_INVALID_RESPONSE, 1);
    reset("unknown-version", 0, 0); run(device, 0, DS_PROBE_UNAVAILABLE, 1);
    reset("response-cntlid", 0, 0); run(device, 3, DS_PROBE_UNAVAILABLE, 1);
    reset("nsid-outside-response", 0, 0); run(device, 3, DS_PROBE_UNAVAILABLE, 1);
    reset("response-nsid", 0, 0); run(device, 0, DS_PROBE_INVALID_RESPONSE, 1);
    reset("close-failure", 0, EIO); run(device, 1, DS_PROBE_UNAVAILABLE, 1); CHECK(closes == 1U);
    reset("close-failure", 0, EINTR); run(device, 1, DS_PROBE_UNAVAILABLE, 1); CHECK(closes == 1U);
    const int resources[] = {ENOMEM, EMFILE, ENFILE};
    for (size_t i = 0; i < sizeof(resources) / sizeof(resources[0]); ++i) {
        reset("open-failure", 0, resources[i]); run(device, 1, DS_PROBE_UNAVAILABLE, 0); CHECK(closes == 0U);
    }
    reset("ioctl-failure", 0, ENOMEM); run(device, 1, DS_PROBE_UNAVAILABLE, 1); CHECK(closes == 1U);
}
/** @brief Verify fixed Linux queries without opening a real controller or submitting any native ioctl. */
int main(void)
{
    fault = "initial"; memset(oversized, 'x', sizeof(oversized) - 1U); oversized[sizeof(oversized) - 1U] = '\0';
    memcpy(oversized, "/dev/", 5);
    ds_device device; selected(&device);
    reset("complete", 0, 0); run(&device, 0, DS_PROBE_COMPLETE, 1); CHECK(maps == 3U && opens == 1U && closes == 1U);
    gating(&device); mapping_faults(&device); native_faults(&device); evidence(&device); ds_device_free(&device);
    printf("Linux fixed protocol: %u scenarios, %u assertions passed.\n", scenarios, checks); return 0;
}
