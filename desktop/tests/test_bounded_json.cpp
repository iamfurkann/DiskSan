/** @file
 * @brief Test exact capacity parsing and adversarial JSON bounds without launching a process or reading devices.
 */
#include "BoundedJson.h"
#include <cstdio>
#include <cstdlib>

using namespace disksan::desktop;
static unsigned checks;
/** @brief Stop at the first contract failure without printing untrusted JSON content. */
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr, "JSON check failed at %d.\n", __LINE__); std::exit(1); } } while (false)

/** @brief Require malformed payloads to leave no partially accepted object or echoed identifier. */
static void rejected(const QByteArray &bytes)
{
    static unsigned caseIndex;
    ++caseIndex;
    auto r = parseJson(bytes);
    if (r.ok) std::fprintf(stderr, "Unexpected acceptance in negative case %u.\n", caseIndex);
    CHECK(!r.ok && !r.error.isEmpty());
    CHECK(r.root.type == JsonType::Null && r.root.object.isEmpty() && r.root.array.isEmpty());
    CHECK(!r.error.contains(QStringLiteral("PRIVATE-IDENTIFIER")));
}
/** @brief Preserve integer precision, JSON types, escaped text and member ownership. */
static void accepted_values()
{
    auto r = parseJson(R"({"capacity":18446744073709551615,"large":9007199254740993,"zero":0,"missing":null,"flag":false,"name":"A\n\u00e9\ud83d\udcbe","items":[true,{},[]]})");
    CHECK(r.ok && r.error.isEmpty());
    CHECK(r.root.object.value("capacity").type == JsonType::Number);
    CHECK(r.root.object.value("capacity").text == QStringLiteral("18446744073709551615"));
    CHECK(r.root.object.value("large").text == QStringLiteral("9007199254740993"));
    CHECK(r.root.object.value("zero").text == QStringLiteral("0"));
    CHECK(r.root.object.value("missing").type == JsonType::Null);
    CHECK(r.root.object.value("flag").type == JsonType::Boolean && !r.root.object.value("flag").boolean);
    CHECK(r.root.object.value("name").text == QString::fromUtf8("A\n\xc3\xa9\xf0\x9f\x92\xbe"));
    CHECK(r.root.object.value("items").array.size() == 3);
    auto text = parseJson(R"({"capacity":"18446744073709551615"})");
    CHECK(text.ok && text.root.object.value("capacity").type == JsonType::String);
    auto controls = parseJson(R"({"name":"\u0000\"\\/\b\f\n\r\t"})");
    CHECK(controls.ok && controls.root.object.value("name").text.size() == 9);
    QByteArray original("{\"name\":\"owned\"}");
    auto owned = parseJson(original); original.fill('X');
    CHECK(owned.ok && owned.root.object.value("name").text == QStringLiteral("owned"));
}
/** @brief Reject duplicate fields, invalid grammar, unsupported numeric forms and invalid Unicode. */
static void malformed()
{
    for (const QByteArray &bytes : {QByteArray(), QByteArray("[]"), QByteArray("null"), QByteArray("{} trailing"),
         QByteArray("{\"PRIVATE-IDENTIFIER\":true,}"), QByteArray("{\"a\":[0,]}"), QByteArray("{\"a\":1 \"b\":2}"),
         QByteArray("{\"a\":0,\"a\":1}"), QByteArray("{\"a\":0,\"\\u0061\":1}"),
         QByteArray("{\"n\":18446744073709551616}"), QByteArray("{\"n\":999999999999999999999}"),
         QByteArray("{\"n\":-1}"), QByteArray("{\"n\":1.0}"), QByteArray("{\"n\":1e2}"),
         QByteArray("{\"n\":01}"), QByteArray("{\"n\":+1}"), QByteArray("{\"n\":NaN}"),
         QByteArray("{\"a\":truefalse}"), QByteArray("{\"a\":\"unterminated}"), QByteArray("{\"a\":\"\\q\"}"),
         QByteArray("{\"a\":\"\\ud800\"}"), QByteArray("{\"a\":\"\\udc00\"}"),
         QByteArray("{\"a\":\"line\nraw\"}"), QByteArray("{\"a\":\"\xff\"}"),
         QByteArray("{\"a\":\"\xc0\x80\"}"), QByteArray("{\"a\":\"\xed\xa0\x80\"}"), QByteArray("{}\xe2\x82")})
        rejected(bytes);
    rejected(QByteArray("{\"a\":\"") + QByteArray(1, '\0') + "\"}");
    QByteArray good("{\"a\":[0,\"text\",true,{\"inner\":false}]}");
    for (qsizetype cut = 0; cut < good.size(); ++cut) rejected(good.left(cut));
}
/** @brief Exercise each explicit resource limit just inside and outside its accepted boundary. */
static void limits()
{
    CHECK(parseJson(QByteArray(JsonByteLimit - 2, ' ') + "{}").ok);
    rejected(QByteArray(JsonByteLimit - 1, ' ') + "{}");
    CHECK(parseJson(QByteArray("{\"a\":\"") + QByteArray(JsonStringLimit, 'x') + "\"}").ok);
    rejected(QByteArray("{\"a\":\"") + QByteArray(JsonStringLimit + 1, 'x') + "\"}");
    QByteArray array("[");
    for (qsizetype i = 0; i < JsonCollectionLimit; ++i) { if (i) array += ','; array += '0'; }
    array += ']';
    CHECK(parseJson(QByteArray("{\"a\":") + array + '}').ok);
    rejected(QByteArray("{\"a\":") + array.chopped(1) + ",0]}");
    CHECK(parseJson(QByteArray("{\"a\":") + QByteArray(JsonDepthLimit - 1, '[') + '0' + QByteArray(JsonDepthLimit - 1, ']') + '}').ok);
    rejected(QByteArray("{\"a\":") + QByteArray(JsonDepthLimit, '[') + '0' + QByteArray(JsonDepthLimit, ']') + '}');
    QByteArray nodes("{\"a\":[");
    for (int i = 0; i < 32; ++i) { if (i) nodes += ','; nodes += array; }
    nodes += "]}"; rejected(nodes);
}
/** @brief Run bounded JSON tests using Qt Core without a display, process or native storage adapter. */
int main()
{
    accepted_values(); malformed(); limits();
    std::printf("Desktop JSON bounds: %u checks passed.\n", checks);
    return 0;
}
