/** @file
 * @brief Reject mismatched CLI schemas, types, authority claims and synthetic provenance before exposing data to QML.
 */
#include "CliContract.h"
#include "BoundedJson.h"
#include "disksan/version.h"
#include <QSet>
#include <new>

namespace disksan::desktop {
/** @brief Retrieve a missing sentinel without mutating the input or confusing absence with explicit null. */
static const JsonValue &member(const JsonValue &value, const char *key)
{
    static const JsonValue missing;
    auto found = value.object.constFind(QString::fromLatin1(key));
    return found == value.object.cend() ? missing : found.value();
}
/** @brief Require an object with exactly the versioned contract's members, including explicitly nullable fields. */
static bool keys(const JsonValue &value, std::initializer_list<const char *> names)
{
    if (value.type != JsonType::Object || value.object.size() != static_cast<qsizetype>(names.size())) return false;
    for (auto key : names) if (!value.object.contains(QString::fromLatin1(key))) return false;
    return true;
}
/** @brief Recognize a bounded string or an explicitly allowed null; never coerce other types. */
static bool text(const JsonValue &value, bool nullable = false)
{
    return value.type == JsonType::String || (nullable && value.type == JsonType::Null);
}
/** @brief Match a literal string without accepting numbers or nulls as labels. */
static bool equals(const JsonValue &value, const char *label)
{
    return value.type == JsonType::String && value.text == QLatin1StringView(label);
}
/** @brief Restrict a status to an explicit set of non-authorizing labels. */
static bool choice(const JsonValue &value, std::initializer_list<const char *> labels)
{
    for (auto label : labels) if (equals(value, label)) return true;
    return false;
}
/** @brief Require an actual JSON boolean with the expected literal value. */
static bool flag(const JsonValue &value, bool expected)
{
    return value.type == JsonType::Boolean && value.boolean == expected;
}
/** @brief Validate a uint64 token whose syntax and overflow were already bounded by the parser. */
static bool integer(const JsonValue &value, bool nullable = false)
{
    return value.type == JsonType::Number || (nullable && value.type == JsonType::Null);
}
/** @brief Require an array and a domain-specific count cap before iterating its members. */
static bool array(const JsonValue &value, qsizetype limit)
{
    return value.type == JsonType::Array && value.array.size() <= limit;
}
/** @brief Validate metadata relationship and warning arrays without interpreting their contents as commands. */
static bool strings(const JsonValue &value)
{
    if (!array(value, 1024)) return false;
    for (const auto &entry : value.array) if (!text(entry)) return false;
    return true;
}
/** @brief Validate every required schema-2 device field before constructing a selectable view record. */
static bool device(const JsonValue &d)
{
    if (!keys(d, {"path","sysfs_path","model","vendor","serial","wwn","eui","nguid","parent","controller","device_id",
                  "major","minor","platform","size_bytes","logical_sector_size","physical_sector_size","rotational","removable",
                  "read_only","device_role","transport","protocol","media_type","media_confidence","classification_source",
                  "sources","stable_paths","holders","slaves","evidence","warnings"})) return false;
    for (auto key : {"path","sysfs_path","model","vendor","serial","wwn","eui","nguid","parent","controller","device_id"})
        if (!text(member(d, key), true)) return false;
    for (auto key : {"platform","device_role","transport","protocol","media_type","media_confidence","classification_source"})
        if (!text(member(d, key))) return false;
    for (auto key : {"major","minor","size_bytes","logical_sector_size","physical_sector_size"})
        if (!integer(member(d, key), true)) return false;
    for (auto key : {"rotational","removable","read_only"}) {
        const auto &v = member(d, key);
        if (v.type != JsonType::Null && v.type != JsonType::Boolean) return false;
    }
    const auto &sources = member(d, "sources");
    if (!keys(sources, {"path","sysfs_path","model","vendor","serial","wwn","eui","nguid","parent","controller","device_id",
                       "size_bytes","logical_sector_size","physical_sector_size","rotational","removable","read_only"})) return false;
    for (const auto &source : sources.object) if (!text(source, true)) return false;
    for (auto key : {"stable_paths","holders","slaves","warnings"}) if (!strings(member(d, key))) return false;
    const auto &evidence = member(d, "evidence");
    if (!array(evidence, 64)) return false;
    for (const auto &entry : evidence.array) {
        if (!keys(entry, {"field","source","value"})) return false;
        for (const auto &field : entry.object) if (!text(field)) return false;
    }
    return true;
}
/** @brief Validate copied observation and rebuilt finding types, preserving unknown codes for display. */
static bool findings(const JsonValue &root)
{
    const auto &observations = member(root, "observations"), &entries = member(root, "findings");
    if (!array(observations, 64) || !array(entries, 64)) return false;
    for (const auto &entry : observations.array) {
        if (!keys(entry, {"check","state","source","detail"})) return false;
        for (const auto &field : entry.object) if (!text(field)) return false;
        if (!choice(member(entry, "state"), {"true","false","unknown"})) return false;
    }
    for (const auto &entry : entries.array) {
        if (!keys(entry, {"code","category","subject","severity","state","source","message"})) return false;
        for (const auto &field : entry.object) if (!text(field)) return false;
        if (!choice(member(entry, "state"), {"true","false","unknown"}) ||
            !choice(member(entry, "severity"), {"info","review","block"})) return false;
    }
    return true;
}
/** @brief Keep all four independent capability states explicit, including unavailability. */
static bool capabilityState(const JsonValue &v)
{
    return choice(v, {"observed_supported","observed_unsupported","unavailable","unknown"});
}
/** @brief Recognize only the six method families described by the CLI schema. */
static bool method(const JsonValue &v)
{
    return choice(v, {"nvme_sanitize_block_erase","nvme_sanitize_crypto_erase","nvme_sanitize_overwrite",
                      "ata_security_erase","ata_sanitize","scsi_sanitize"});
}
/** @brief Validate one raw capability observation with independent dimension and bounded provenance. */
static bool capabilityEvidence(const JsonValue &v, bool withMethod)
{
    if (withMethod ? !keys(v, {"method","dimension","state","source","detail"}) :
                     !keys(v, {"dimension","state","source","detail"})) return false;
    return (!withMethod || method(member(v, "method"))) &&
        choice(member(v, "dimension"), {"support","os_access","applicability","passthrough"}) &&
        capabilityState(member(v, "state")) && text(member(v, "source")) && text(member(v, "detail"));
}
/** @brief Validate all method summaries and their bounded independent evidence without guessing capabilities. */
static bool capabilities(const JsonValue &root)
{
    const auto &caps = member(root, "capabilities");
    if (!array(caps, 6) || caps.array.size() != 6) return false;
    QSet<QString> methods; qsizetype count = 0;
    for (const auto &entry : caps.array) {
        if (!keys(entry, {"method","support","os_access","applicability","passthrough","evidence"}) ||
            !method(member(entry, "method"))) return false;
        auto name = member(entry, "method").text;
        if (methods.contains(name)) return false;
        methods.insert(name);
        for (auto key : {"support","os_access","applicability","passthrough"}) if (!capabilityState(member(entry, key))) return false;
        const auto &evidence = member(entry, "evidence");
        if (!array(evidence, 64) || (count += evidence.array.size()) > 64) return false;
        for (const auto &e : evidence.array) if (!capabilityEvidence(e, false)) return false;
    }
    return true;
}
/** @brief Require bounded fixed prerequisites and a transcript whose states remain modeled and non-executable. */
static bool planning(const JsonValue &root, bool simulation, bool synthetic)
{
    if (!flag(member(root, "simulation_only"), true) || !flag(member(root, "execution_available"), false) ||
        !flag(member(root, "verification_available"), false) || !flag(member(root, "privilege_unknown"), true) ||
        member(root, "physical_scope_unknown").type != JsonType::Boolean || member(root, "target_observed").type != JsonType::Boolean ||
        !equals(member(root, "evidence_origin"), synthetic ? "synthetic_fixture" : "native_read_only_snapshot")) return false;
    const auto &raw = member(root, "method_evidence"), &prerequisites = member(root, "prerequisites"), &steps = member(root, "steps");
    if (!array(raw, 64) || !array(prerequisites, 10) || prerequisites.array.size() != 10 || !array(steps, 10) ||
        steps.array.size() != (simulation ? 10 : 0)) return false;
    for (const auto &entry : raw.array) if (!capabilityEvidence(entry, true)) return false;
    static const char *const required[] = {"target_observed","identity_confidence","usage_status","topology_coverage","method_evidence",
        "privilege_unknown","independent_authorization_unimplemented","exclusive_access_unimplemented","execution_unavailable","verification_unavailable"};
    static const char *const codes[] = {"observe_target","compare_identity","review_usage","review_topology","review_method_evidence",
        "privilege_boundary_unimplemented","authorization_boundary_unimplemented","exclusive_scope_unimplemented",
        "operation_backend_unavailable","completion_verifier_unavailable"};
    for (qsizetype i = 0; i < 10; ++i) {
        const auto &p = prerequisites.array[i];
        if (!keys(p, {"modeled_only","code","state","evidence_scope","explanation"}) || !flag(member(p, "modeled_only"), true) ||
            !equals(member(p, "code"), required[i]) || !text(member(p, "evidence_scope")) || !text(member(p, "explanation")) ||
            !choice(member(p, "state"), {"inspection_step_modeled","would_block","unknown_requires_review","not_implemented"}) ||
            (i >= 5 && !equals(member(p, "state"), "not_implemented"))) return false;
        if (simulation) {
            const auto &step = steps.array[i];
            if (!keys(step, {"index","modeled_only","code","state","prerequisite","evidence_scope","explanation"}) ||
                !integer(member(step, "index")) || member(step, "index").text != QString::number(i + 1) ||
                !flag(member(step, "modeled_only"), true) || !equals(member(step, "code"), codes[i]) ||
                !equals(member(step, "prerequisite"), required[i])) return false;
            for (auto key : {"state","evidence_scope","explanation"})
                if (!text(member(step, key)) || member(step, key).text != member(p, key).text) return false;
        }
    }
    return !simulation || equals(member(root, "simulation_result"), "non_executable_model_only");
}
/** @brief Convert already validated values to QML-friendly types without passing integer capacities through double. */
static QVariant displayValue(const JsonValue &value)
{
    switch (value.type) {
        case JsonType::Null: return {};
        case JsonType::Boolean: return value.boolean;
        case JsonType::Number: case JsonType::String: return value.text;
        case JsonType::Array: {
            QVariantList list;
            for (const auto &item : value.array) list.append(displayValue(item));
            return list;
        }
        case JsonType::Object: {
            QVariantMap map;
            for (auto i = value.object.cbegin(); i != value.object.cend(); ++i) map.insert(i.key(), displayValue(i.value()));
            return map;
        }
    }
    return {};
}
/** @brief Map an enum to fixed argv; opaque selectors never participate in command construction. */
QStringList commandArguments(Command command, const QString &selector)
{
    if (command == Command::Version) return selector.isEmpty() ? QStringList{"--version"} : QStringList{};
    if (command == Command::List) return selector.isEmpty() ? QStringList{"list","--all","--json"} : QStringList{};
    if (selector.isEmpty() || selector.contains(QChar::Null) || selector.toUtf8().size() > 4096) return {};
    QString name;
    switch (command) {
        case Command::Info: name = QStringLiteral("info"); break;
        case Command::Assess: name = QStringLiteral("assess"); break;
        case Command::Capabilities: name = QStringLiteral("capabilities"); break;
        case Command::Plan: name = QStringLiteral("plan"); break;
        case Command::Simulate: name = QStringLiteral("simulate"); break;
        default: return {};
    }
    return {name, QStringLiteral("--json"), QStringLiteral("--"), selector};
}
/** @brief Enforce exact version, operation, schema, provenance and non-authorizing flags before publishing a typed response. */
CliReply validateReply(Command command, const QByteArray &bytes, bool synthetic)
{
    CliReply reply;
    reply.error = QStringLiteral("CLI response did not match the expected read-only schema; refresh before retrying.");
    if (command == Command::Version) {
        reply.ok = bytes == QByteArray("DiskSan v" DISKSAN_VERSION "\n") || bytes == QByteArray("DiskSan v" DISKSAN_VERSION "\r\n");
        if (reply.ok) reply.error.clear();
        return reply;
    }
    try {
        auto parsed = parseJson(bytes);
        if (!parsed.ok) { reply.error = parsed.error; return reply; }
        const auto &root = parsed.root;
        if (!integer(member(root, "schema_version")) || member(root, "schema_version").text != "2" ||
            !equals(member(root, "application"), "DiskSan") || !equals(member(root, "version"), DISKSAN_VERSION) ||
            !equals(member(root, "mode"), "read-only") || root.object.contains("probe")) return reply;
        if (command == Command::List || command == Command::Info) {
            if (!keys(root, {"schema_version","application","version","mode","snapshot","devices","shown","omitted","warnings"}) ||
                !flag(member(root, "snapshot"), true) || !strings(member(root, "warnings"))) return reply;
            const auto &records = member(root, "devices");
            if (!array(records, 256) || (command == Command::Info && records.array.size() != 1) ||
                !integer(member(root, "shown")) || member(root, "shown").text != QString::number(records.array.size()) ||
                !integer(member(root, "omitted")) || member(root, "omitted").text != "0") return reply;
            QSet<QString> paths, identities;
            for (const auto &d : records.array) {
                if (!device(d)) return reply;
                QString platform = member(d, "platform").text;
                for (auto field : {"path","device_id"}) {
                    const auto &v = member(d, field);
                    if (v.type == JsonType::Null || v.text.isEmpty()) continue;
                    QString key = platform + QChar::Null + v.text;
                    auto &set = qstrcmp(field, "path") == 0 ? paths : identities;
                    if (set.contains(key)) return reply;
                    set.insert(key);
                }
            }
        } else {
            bool plan = command == Command::Plan || command == Command::Simulate;
            auto args = commandArguments(command, QStringLiteral("schema-selector"));
            if (args.isEmpty() || !text(member(root, "operation")) || member(root, "operation").text != args[0] ||
                !device(member(root, "device")) || !flag(member(root, "erase_authorized"), false) || !findings(root)) return reply;
            const auto &identity = member(root, "identity");
            if (!keys(identity, {"status","atomic","guarantees_future_identity","valid_until"}) ||
                !flag(member(identity, "atomic"), false) || !flag(member(identity, "guarantees_future_identity"), false) ||
                !equals(member(identity, "valid_until"), "end_of_observation")) return reply;
            if (plan) {
                if (!choice(member(identity, "status"), {"observed_for_snapshot","incomplete"}) ||
                    !choice(member(root, "inspection_status"), {"blocked","review_required","insufficient_evidence","inspection_only"}) ||
                    !planning(root, command == Command::Simulate, synthetic)) return reply;
                qsizetype expected = command == Command::Simulate ? 22 : 21;
                if (root.object.size() != expected || (command == Command::Plan && root.object.contains("simulation_result"))) return reply;
            } else {
                if (!keys(root, {"schema_version","application","version","mode","snapshot","operation","assessment_kind","device",
                                 "identity","inspection_status","erase_authorized","observations","findings","capabilities"}) ||
                    !flag(member(root, "snapshot"), true) ||
                    !equals(member(root, "assessment_kind"), command == Command::Assess ? "target-preflight" : "capability-evidence") ||
                    !choice(member(identity, "status"), {"verified_for_snapshot","partially_verified"}) ||
                    !choice(member(root, "inspection_status"), {"blocked","review_required","no_blockers_observed_in_snapshot"}) ||
                    !capabilities(root)) return reply;
            }
        }
        reply.data = displayValue(root).toMap(); reply.ok = true; reply.error.clear();
    } catch (const std::bad_alloc &) {
        reply.data.clear(); reply.ok = false;
        reply.error = QStringLiteral("Insufficient memory to validate CLI data.");
    }
    return reply;
}
/** @brief Compare only already validated snapshot identities and relevant scope/state without claiming future stability. */
bool sameDevice(const QVariantMap &selected, const QVariantMap &observed)
{
    if (selected.value("path").toString().isEmpty()) return false;
    for (auto key : {"platform","device_id","path","device_role","major","minor","size_bytes","read_only",
                     "logical_sector_size","physical_sector_size","parent","controller","holders","slaves"})
        if (!selected.contains(key) || !observed.contains(key) || selected.value(key) != observed.value(key)) return false;
    return true;
}
}
