/** @file
 * @brief Drive desktop contract tests with bounded stdin fixtures and exercise fixed argv without launching programs.
 */
#include "CliContract.h"
#include "BoundedJson.h"
#include <QJsonDocument>
#include <cstdio>
#include <cstdlib>

using namespace disksan::desktop;
/** @brief Fail a synthetic contract assertion without printing device data. */
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "Contract check failed at %d.\n", __LINE__); std::exit(1); } } while (false)
/** @brief Prove fixed argv and identity checks reject invalid operation and selector combinations. */
static void selfTest()
{
    CHECK(commandArguments(Command::Version) == QStringList{"--version"});
    CHECK((commandArguments(Command::List) == QStringList{"list","--all","--json"}));
    CHECK(commandArguments(Command::List, "unexpected").isEmpty());
    CHECK(commandArguments(Command::Version, "unexpected").isEmpty());
    CHECK(commandArguments(static_cast<Command>(99), "device").isEmpty());
    for (Command command : {Command::Info,Command::Assess,Command::Capabilities,Command::Plan,Command::Simulate}) {
        CHECK(commandArguments(command).isEmpty());
        CHECK(commandArguments(command, QString(4097, 'x')).isEmpty());
        CHECK(commandArguments(command, QString("bad") + QChar::Null + "selector").isEmpty());
        for (const QString &selector : {QString("--probe"),QString("$(false);`false`"),QString("with spaces"),QString("line\nname")}) {
            auto args = commandArguments(command, selector);
            CHECK(args.size() == 4 && args[1] == "--json" && args[2] == "--" && args[3] == selector);
            CHECK(!args[0].contains("probe") && !args[0].contains("execute"));
        }
    }
    QVariantMap selected;
    for (auto key : {"platform","device_id","path","device_role","major","minor","size_bytes","read_only",
                     "logical_sector_size","physical_sector_size","parent","controller","holders","slaves"}) selected[key] = QVariant{};
    selected["path"] = "synthetic";
    CHECK(sameDevice(selected, selected));
    for (auto i = selected.cbegin(); i != selected.cend(); ++i) {
        auto changed = selected; changed[i.key()] = QString("changed"); CHECK(!sameDevice(selected, changed));
        changed = selected; changed.remove(i.key()); CHECK(!sameDevice(selected, changed));
    }
    CHECK(!sameDevice({}, {}));
}
/** @brief Accept only fixture-mode test arguments and bounded stdin, returning validated display JSON for assertions. */
int main(int argc, char **argv)
{
    if (argc == 2 && QByteArray(argv[1]) == "--self-test") { selfTest(); return 0; }
    if (argc != 3 || (QByteArray(argv[2]) != "0" && QByteArray(argv[2]) != "1")) return 2;
    QStringList names{"version","list","info","assess","capabilities","plan","simulate"};
    qsizetype index = names.indexOf(QString::fromUtf8(argv[1]));
    if (index < 0) return 2;
    QByteArray bytes;
    char chunk[8192]; size_t count;
    while ((count = std::fread(chunk, 1, sizeof(chunk), stdin)) != 0U) {
        if (bytes.size() + static_cast<qsizetype>(count) > JsonByteLimit) return 1;
        bytes.append(chunk, static_cast<qsizetype>(count));
    }
    if (std::ferror(stdin)) return 1;
    auto reply = validateReply(static_cast<Command>(index), bytes, QByteArray(argv[2]) == "1");
    if (!reply.ok) { CHECK(reply.data.isEmpty()); return 1; }
    auto out = QJsonDocument::fromVariant(reply.data).toJson(QJsonDocument::Compact);
    return std::fwrite(out.data(), 1, static_cast<size_t>(out.size()), stdout) == static_cast<size_t>(out.size()) &&
        std::fflush(stdout) == 0 ? 0 : 1;
}
