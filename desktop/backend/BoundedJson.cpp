/** @file
 * @brief Bound JSON structure before consumption and avoid floating-point conversion of device capacities.
 */
#include "BoundedJson.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDeadlineTimer>
#include <new>

namespace disksan::desktop {
/** @brief Keep parse offsets and counters local to one bounded borrowed payload. */
struct ParseState {
    const QByteArray &bytes; /**< @brief Borrowed immutable input for this call only. */
    qsizetype position = 0; /**< @brief Next byte position, never beyond input length. */
    qsizetype nodes = 0; /**< @brief Number of values visited across all nested containers. */
    QDeadlineTimer deadline{JsonParseDeadlineMs}; /**< @brief Monotonic work deadline checked during traversal. */
};
/** @brief Recognize only JSON whitespace rather than locale-dependent character classes. */
static bool whitespace(ParseState &s)
{
    const char *const data = s.bytes.constData();
    const qsizetype size = s.bytes.size();
    qsizetype position = s.position;
    while (position < size) {
        if ((position & 4095) == 0 && s.deadline.hasExpired()) return false;
        const char c = data[position];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') break;
        ++position;
    }
    s.position = position;
    return true;
}
/** @brief Consume one structural byte without advancing on mismatch. */
static bool consume(ParseState &s, char expected)
{
    if (!whitespace(s)) return false;
    if (s.position == s.bytes.size() || s.bytes[s.position] != expected) return false;
    ++s.position; return true;
}
/** @brief Read exactly four ASCII hexadecimal digits from a JSON Unicode escape. */
static bool unicodeUnit(ParseState &s, unsigned &unit)
{
    if (s.bytes.size() - s.position < 4) return false;
    unit = 0;
    for (int i = 0; i < 4; ++i) {
        char c = s.bytes[s.position++]; unsigned digit;
        if (c >= '0' && c <= '9') digit = static_cast<unsigned>(c - '0');
        else if (c >= 'a' && c <= 'f') digit = static_cast<unsigned>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') digit = static_cast<unsigned>(c - 'A' + 10);
        else return false;
        unit = (unit << 4U) | digit;
    }
    return true;
}
/** @brief Decode a bounded quoted string after strict escape and surrogate checks, avoiding permissive Qt escape acceptance. */
static bool string(ParseState &s, QString &result)
{
    if (!whitespace(s)) return false;
    qsizetype start = s.position;
    if (!consume(s, '"')) return false;
    const char *const data = s.bytes.constData();
    const qsizetype size = s.bytes.size();
    bool plainAscii = true;
    bool closed = false;
    while (s.position < size && s.position - start <= JsonStringLimit * 6 + 2) {
        unsigned char c = static_cast<unsigned char>(data[s.position++]);
        if (c < 0x20U) return false;
        if (c >= 0x80U) plainAscii = false;
        if (c == '\\') {
            plainAscii = false;
            if (s.position == size) return false;
            char escape = data[s.position++];
            if (escape == 'u') {
                unsigned unit;
                if (!unicodeUnit(s, unit) || (unit >= 0xdc00U && unit <= 0xdfffU)) return false;
                if (unit >= 0xd800U && unit <= 0xdbffU) {
                    if (size - s.position < 6 || s.bytes[s.position] != '\\' || s.bytes[s.position + 1] != 'u') return false;
                    s.position += 2;
                    if (!unicodeUnit(s, unit) || unit < 0xdc00U || unit > 0xdfffU) return false;
                }
            } else if (escape != '"' && escape != '\\' && escape != '/' && escape != 'b' && escape != 'f' &&
                       escape != 'n' && escape != 'r' && escape != 't') return false;
            continue;
        }
        if (c == '"') { closed = true; break; }
    }
    if (!closed) return false;
    // Plain ASCII has no escape decoding and its byte length is already the UTF-8 length.
    if (plainAscii) {
        const qsizetype length = s.position - start - 2;
        if (length > JsonStringLimit) return false;
        result = QString::fromLatin1(data + start + 1, length);
        return true;
    }
    QByteArray quoted("["); quoted += s.bytes.mid(start, s.position - start); quoted += ']';
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(quoted, &error);
    if (error.error != QJsonParseError::NoError || !doc.isArray() || doc.array().size() != 1 || !doc.array()[0].isString()) return false;
    result = doc.array()[0].toString();
    if (!result.isValidUtf16() || result.toUtf8().size() > JsonStringLimit) return false;
    return true;
}
/** @brief Preserve only canonical unsigned decimal tokens within uint64 bounds. */
static bool number(ParseState &s, JsonValue &value)
{
    qsizetype start = s.position;
    while (s.position < s.bytes.size() && s.bytes[s.position] >= '0' && s.bytes[s.position] <= '9') ++s.position;
    qsizetype size = s.position - start;
    if (size == 0 || size > 20 || (size > 1 && s.bytes[start] == '0')) return false;
    QByteArray token = s.bytes.mid(start, size);
    if (size == 20 && token > QByteArrayLiteral("18446744073709551615")) return false;
    value.type = JsonType::Number; value.text = QString::fromLatin1(token); return true;
}
/** @brief Recursively parse only bounded values, rejecting duplicate members and trailing separators. */
static bool value(ParseState &s, JsonValue &out, qsizetype depth)
{
    if (depth > JsonDepthLimit || ++s.nodes > JsonNodeLimit || s.deadline.hasExpired()) return false;
    if (!whitespace(s)) return false;
    if (s.position == s.bytes.size()) return false;
    char c = s.bytes[s.position];
    if (c == '"') { out.type = JsonType::String; return string(s, out.text); }
    if (c >= '0' && c <= '9') return number(s, out);
    if (c == '{' || c == '[') {
        ++s.position;
        bool object = c == '{'; char close = object ? '}' : ']';
        out.type = object ? JsonType::Object : JsonType::Array;
        if (consume(s, close)) return true;
        for (qsizetype count = 0; count < JsonCollectionLimit; ++count) {
            QString key;
            if (object && (!string(s, key) || out.object.contains(key) || !consume(s, ':'))) return false;
            JsonValue child;
            if (!value(s, child, depth + 1)) return false;
            if (object) out.object.insert(key, std::move(child)); else out.array.append(std::move(child));
            if (consume(s, close)) return true;
            if (!consume(s, ',')) return false;
        }
        return false;
    }
    for (const auto &literal : {QByteArrayLiteral("null"), QByteArrayLiteral("true"), QByteArrayLiteral("false")}) {
        if (s.bytes.mid(s.position, literal.size()) != literal) continue;
        s.position += literal.size();
        out.type = literal == "null" ? JsonType::Null : JsonType::Boolean;
        out.boolean = literal == "true"; return true;
    }
    return false;
}
/** @brief Return only a fully parsed bounded object and suppress untrusted input in every error message. */
JsonResult parseJson(const QByteArray &bytes)
{
    JsonResult result;
    result.error = QStringLiteral("Invalid or oversized CLI JSON; no device results were accepted.");
    if (bytes.isEmpty() || bytes.size() > JsonByteLimit || !bytes.isValidUtf8()) return result;
    try {
        ParseState state{bytes}; JsonValue root;
        if (!value(state, root, 0) || root.type != JsonType::Object) return result;
        if (!whitespace(state) || state.deadline.hasExpired() || state.position != bytes.size()) return result;
        result.root = std::move(root); result.ok = true; result.error.clear();
    } catch (const std::bad_alloc &) {
        result = JsonResult{};
        result.error = QStringLiteral("Insufficient memory to validate CLI output; no device results were accepted.");
    }
    return result;
}
}
