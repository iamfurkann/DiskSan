/** @file
 * @brief Emulate a hostile or delayed CLI process with synthetic build-local data and no native storage APIs.
 */
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTimer>
#include <cstdio>

/** @brief Emit bounded fixture bytes and a chosen exit class without printing sensitive observations. */
static int send(const QByteArray &data, const QByteArray &error, int status)
{
    if (std::fwrite(data.data(), 1, static_cast<size_t>(data.size()), stdout) != static_cast<size_t>(data.size())) return 97;
    if (std::fwrite(error.data(), 1, static_cast<size_t>(error.size()), stderr) != static_cast<size_t>(error.size())) return 97;
    if (std::fflush(stdout) || std::fflush(stderr)) return 97;
    return status;
}
/** @brief Accept only the desktop allowlist, then apply explicitly test-only faults to static fixture responses. */
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments().mid(1);
    QString command, selector;
    if (args == QStringList{"--version"}) command = "version";
    else if (args == QStringList{"list", "--all", "--json"}) command = "list";
    else if (args.size() == 4 && args[1] == "--json" && args[2] == "--" &&
             QStringList{"info","assess","capabilities","plan","simulate"}.contains(args[0])) {
        command = args[0]; selector = args[3];
    } else return 99;
    QByteArray mode = qgetenv("DISKSAN_GUI_TEST_MODE");
    QString log = QString::fromUtf8(qgetenv("DISKSAN_GUI_TEST_LOG"));
    if (!log.isEmpty()) {
        QFile file(log);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) return 96;
        QJsonArray array;
        for (const auto &arg : args) array.append(arg);
        file.write(QJsonDocument(array).toJson(QJsonDocument::Compact) + '\n');
    }
    QString fixture = mode == "empty" && command == "list" ? "empty" : command;
    QFile file(QString::fromUtf8(DISKSAN_FIXTURE_DIR) + '/' + fixture + ".json");
    if (!file.open(QIODevice::ReadOnly)) return 95;
    QByteArray data = file.readAll(), error;
    int status = 0;
    if (selector == "second" || selector == "--probe") {
        data.replace("\"complete\"", '"' + selector.toUtf8() + '"');
        data.replace("synthetic#diskseq=7", selector == "second" ? "synthetic#diskseq=8" : "synthetic#diskseq=9");
    }
    if (mode == "version" && command == "version") data = "DiskSan v0.0.0\n";
    if (command != "version" && command != "list") {
        if (mode == "malformed") data = "{invalid";
        if (mode == "stale") data.replace("\"complete\"", "\"replaced\"");
        if (mode == "stdout") data = QByteArray(2 * 1024 * 1024 + 4096, 'X');
        if (mode == "stderr") error = QByteArray(32769, 'X');
        if (mode == "denied") { status = 4; error = "PRIVATE-IDENTIFIER\n"; }
        if (mode == "nonzero") status = 3;
    }
    bool delay = !selector.isEmpty() && (mode == "timeout" || (mode == "delay-first" && selector == "complete"));
    if (!delay) return send(data, error, status);
    QTimer::singleShot(2000, &app, [&app, data, error, status] { app.exit(send(data, error, status)); });
    return app.exec();
}
