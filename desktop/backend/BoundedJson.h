/** @file
 * @brief Parse bounded untrusted CLI JSON while preserving unsigned integers exactly and rejecting duplicate keys.
 */
#ifndef DISKSAN_DESKTOP_BOUNDED_JSON_H
#define DISKSAN_DESKTOP_BOUNDED_JSON_H
#include <QByteArray>
#include <QList>
#include <QMap>
#include <QString>

namespace disksan::desktop {
/** @brief Maximum accepted CLI stdout bytes, independent of process exit status. */
inline constexpr qsizetype JsonByteLimit = 2 * 1024 * 1024;
/** @brief Maximum decoded UTF-8 bytes in one field, including escaped control metadata. */
inline constexpr qsizetype JsonStringLimit = 16 * 1024;
/** @brief Fixed parser node, nesting and collection limits bound memory and recursion. */
inline constexpr qsizetype JsonNodeLimit = 32768, JsonDepthLimit = 16, JsonCollectionLimit = 1024;
/** @brief Cooperative monotonic decoding deadline; timed-out input is discarded rather than published. */
inline constexpr int JsonParseDeadlineMs = 500;
/** @brief Preserve number/string/null distinctions before applying schema-specific contracts. */
enum class JsonType { Null, Boolean, Number, String, Object, Array };
/** @brief Own a bounded value; unsigned number tokens remain exact decimal text, never doubles. */
struct JsonValue {
    JsonType type = JsonType::Null; /**< @brief Original JSON type, including explicit null. */
    QString text; /**< @brief Decoded string or exact unsigned number token. */
    bool boolean = false; /**< @brief Meaningful only for Boolean values. */
    QMap<QString, JsonValue> object; /**< @brief Owned members with duplicate keys rejected. */
    QList<JsonValue> array; /**< @brief Owned ordered array, limited during parsing. */
};
/** @brief Publish a complete parsed object or a static redacted error, never partial data. */
struct JsonResult {
    bool ok = false; /**< @brief True only after full syntax and resource validation. */
    JsonValue root; /**< @brief Owned object on success; empty null on failure. */
    QString error; /**< @brief Static explanation without raw device data. */
};
/**
 * @brief Parse the CLI's unsigned-integer JSON subset with duplicate, Unicode and resource checks.
 * @param[in] bytes Borrowed complete stdout bytes; empty and oversized payloads are rejected.
 * @return Owned complete object on success, otherwise an empty result and redacted error.
 * @note Reject fractions, exponents, negative numbers and integers above UINT64_MAX; no current CLI field uses them.
 */
JsonResult parseJson(const QByteArray &bytes);
}
#endif
