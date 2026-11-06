/** @file
 * @brief Launch the optional read-only Qt Quick desktop with one build-pinned CLI executable.
 */
#include "DeviceService.h"
#include "disksan/version.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <cstdio>

/** @brief Construct the trusted service and QML window without accepting a program path, shell command or automatic probe. */
int main(int argc, char **argv)
{
    // Qt consumes platform and plugin options during application construction.
    if (argc != 1) {
        std::fputs("DiskSan Desktop is an inspection-only graphical prototype. Launch without arguments.\n", stderr);
        return 2;
    }
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("DiskSan Desktop"));
    app.setApplicationVersion(QStringLiteral(DISKSAN_VERSION));
    QQuickStyle::setStyle(QStringLiteral("Fusion"));
#ifdef DISKSAN_DESKTOP_FIXTURE
    constexpr bool demo = true;
#else
    constexpr bool demo = false;
#endif
    disksan::desktop::DeviceService service(QString::fromUtf8(DISKSAN_CLI_PATH), demo);
    qmlRegisterUncreatableType<disksan::desktop::DeviceService>("DiskSan.Backend", 1, 0, "DeviceService",
                                                            "The application owns the pinned read-only service.");
    QQmlApplicationEngine engine;
    engine.setInitialProperties({{QStringLiteral("service"), QVariant::fromValue(&service)},
                                 {QStringLiteral("versionText"), QStringLiteral(DISKSAN_VERSION)}});
    engine.load(QUrl(QStringLiteral("qrc:/disksan/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) return 1;
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    if (!window) return 1;
    QObject::connect(&service, &disksan::desktop::DeviceService::stopped, &app, [&app, window] {
        window->setProperty("closingReady", true); app.quit();
    });
    return app.exec();
}
