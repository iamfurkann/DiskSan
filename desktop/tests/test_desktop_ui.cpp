/** @file
 * @brief Run the actual QML window through fixture and separately labeled native read-only inspection journeys.
 */
#include "DeviceService.h"
#include "disksan/version.h"
#include <QQmlApplicationEngine>
#include <QQmlProperty>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickStyle>
#include <QAccessible>
#include <QtTest>

using namespace disksan::desktop;
/** @brief Exercise rendered QML controls while keeping synthetic screenshots separate from live device observations. */
class DesktopTests final : public QObject {
    Q_OBJECT
private:
    /** @brief Find a rendered delegate through visual parents, which need not match QObject ownership. */
    QQuickItem *find(QQuickItem *item, const char *name)
    {
        if (item->objectName() == QLatin1StringView(name)) return item;
        for (auto *child : item->childItems()) if (auto *match = find(child, name)) return match;
        return nullptr;
    }
    /** @brief Load the production resource with a trusted service supplied as its required initial property. */
    QQuickWindow *load(QQmlApplicationEngine &engine, DeviceService &service)
    {
        engine.setInitialProperties({{"service", QVariant::fromValue(&service)}, {"versionText", QStringLiteral(DISKSAN_VERSION)}});
        engine.load(QUrl("qrc:/disksan/qml/Main.qml"));
        return engine.rootObjects().isEmpty() ? nullptr : qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    }
    /** @brief Click a real visible enabled QML control through window coordinates. */
    void click(QQuickWindow *window, const char *name)
    {
        auto *item = find(window->contentItem(), name);
        QVERIFY(item); QVERIFY(item->isVisible()); QVERIFY(item->isEnabled());
        QPointF center = item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
    }
    /** @brief Traverse the real tab chain and activate a named control without setting focus directly. */
    void keyboardActivate(QQuickWindow *window, const char *name)
    {
        window->requestActivate(); QVERIFY(QTest::qWaitForWindowActive(window));
        auto *item = find(window->contentItem(), name); QVERIFY(item);
        QVERIFY(item->isVisible()); QVERIFY(item->isEnabled());
        QStringList focusPath;
        for (int count = 0; count < 64 && !item->hasActiveFocus(); ++count) {
            QTest::keyClick(window, Qt::Key_Tab);
            QCoreApplication::processEvents();
            focusPath.append(window->activeFocusItem() ? window->activeFocusItem()->objectName() : QStringLiteral("none"));
        }
        QString focusFailure = QString::fromLatin1(name) + " / current focus: " +
            (window->activeFocusItem() ? window->activeFocusItem()->objectName() : QStringLiteral("none")) +
            " / tab path: " + focusPath.mid(0, 16).join(",");
        if (!item->hasActiveFocus()) {
            auto *nav = find(window->contentItem(), "navigation0");
            auto next = QQmlProperty::read(nav, "KeyNavigation.tab", qmlContext(nav));
            auto *object = next.value<QObject *>();
            focusFailure += " / navigation target: " + (object ? object->objectName() : QStringLiteral("null"));
            auto *about = find(window->contentItem(), "navigation3");
            focusFailure += QStringLiteral(" / about enabled/visible: %1/%2").arg(about->isEnabled()).arg(about->isVisible());
        }
        QVERIFY2(item->hasActiveFocus(), qPrintable(focusFailure));
        auto *accessible = QAccessible::queryAccessibleInterface(item); QVERIFY(accessible);
        QVERIFY2(!accessible->text(QAccessible::Name).isEmpty(), name);
        QVERIFY(!accessible->state().disabled);
        QTest::keyClick(window, Qt::Key_Space);
    }
private slots:
    /** @brief Register the immutable backend type once and use a deterministic supported controls style. */
    void initTestCase()
    {
        QQuickStyle::setStyle("Fusion");
        QGuiApplication::setQuitOnLastWindowClosed(false);
        qmlRegisterUncreatableType<DeviceService>("DiskSan.Backend", 1, 0, "DeviceService", "Test-owned service.");
    }
    /** @brief Reset the synthetic child scenario before each independent QML journey. */
    void init() { qputenv("DISKSAN_GUI_TEST_MODE", "normal"); }
    /** @brief Remove test overrides after each case. */
    void cleanup() { qunsetenv("DISKSAN_GUI_TEST_MODE"); }
    /** @brief Navigate the real QML controls with keyboard and pointer input and capture only the labeled synthetic view. */
    void fixtureJourney()
    {
        DeviceService service(QString::fromUtf8(DISKSAN_PROCESS_FIXTURE), true);
        QQmlApplicationEngine engine;
        QSignalSpy warnings(&engine, &QQmlEngine::warnings);
        auto *window = load(engine, service); QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowExposed(window));
        QCOMPARE(service.state(), "Disconnected"); QVERIFY(service.devices().isEmpty());
        auto *refresh = find(window->contentItem(), "refreshButton"); QVERIFY(refresh);
        refresh->forceActiveFocus(); QVERIFY(refresh->hasActiveFocus());
        QTest::keyClick(window, Qt::Key_Space);
        QTRY_VERIFY_WITH_TIMEOUT(!service.busy() && service.devices().size() == 3, 5000);
        QTRY_VERIFY_WITH_TIMEOUT(find(window->contentItem(), "deviceRow0") != nullptr, 3000);
        click(window, "deviceRow0");
        QTRY_VERIFY_WITH_TIMEOUT(service.fresh() && !service.busy(), 5000);
        click(window, "navigation1"); QCOMPARE(window->property("page").toInt(), 1);
        QVERIFY(!service.assessment().isEmpty());
        auto *path = find(window->contentItem(), "selectedPath"); QVERIFY(path);
        QVERIFY(path->property("readOnly").toBool()); QVERIFY(path->property("selectByMouse").toBool());
        QVERIFY(QMetaObject::invokeMethod(path, "selectAll"));
        QCOMPARE(path->property("selectedText").toString(), QStringLiteral("complete"));
        click(window, "navigation2"); QCOMPARE(window->property("page").toInt(), 2);
        click(window, "planButton"); QTRY_VERIFY_WITH_TIMEOUT(!service.busy() && !service.plan().isEmpty(), 5000);
        click(window, "simulationButton"); QTRY_VERIFY_WITH_TIMEOUT(!service.busy() && !service.simulation().isEmpty(), 5000);
        auto *banner = find(window->contentItem(), "simulationBanner"); QVERIFY(banner);
        QVERIFY(banner->isVisible()); QVERIFY(banner->property("text").toString().contains("NO STORAGE OPERATION WILL RUN"));
        QCOMPARE(banner->property("textFormat").toInt(), static_cast<int>(Qt::PlainText));
        auto *demo = find(window->contentItem(), "demoBanner"); QVERIFY(demo); QVERIFY(demo->isVisible());
        QTest::keyClick(window, Qt::Key_Tab);
        QVERIFY(window->activeFocusItem() != nullptr);
        QString image = QString::fromUtf8(qgetenv("DISKSAN_TEST_SCREENSHOT"));
        if (!image.isEmpty()) {
            QTest::qWait(100);
            auto screenshot = window->grabWindow(); QVERIFY(!screenshot.isNull()); QVERIFY(screenshot.save(image));
        }
        QCOMPARE(warnings.size(), 0);
        service.shutdown(); window->setProperty("closingReady", true); window->close();
    }
    /** @brief Keep empty, failed and unavailable states visibly distinct, with no fabricated inventory. */
    void invalidStates()
    {
        for (const QByteArray &mode : {QByteArray("empty"), QByteArray("version")}) {
            qputenv("DISKSAN_GUI_TEST_MODE", mode);
            DeviceService service(QString::fromUtf8(DISKSAN_PROCESS_FIXTURE), true);
            QQmlApplicationEngine engine; auto *window = load(engine, service); QVERIFY(window);
            QVERIFY(QTest::qWaitForWindowExposed(window));
            click(window, "refreshButton"); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 5000);
            QVERIFY(service.devices().isEmpty());
            QCOMPARE(service.state(), mode == "empty" ? "Empty" : "Error");
            auto *navigation = find(window->contentItem(), "navigation2"); QVERIFY(navigation); QVERIFY(!navigation->isEnabled());
            service.shutdown(); window->setProperty("closingReady", true); window->close();
        }
        DeviceService missing(QStringLiteral("/disksan-test-unavailable/no-cli"));
        QQmlApplicationEngine engine; auto *window = load(engine, missing); QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowExposed(window));
        click(window, "refreshButton"); QCOMPARE(missing.state(), "Error");
        QVERIFY(missing.devices().isEmpty()); QVERIFY(!missing.demo());
        QVERIFY(!find(window->contentItem(), "demoBanner")->isVisible());
        missing.shutdown(); window->setProperty("closingReady", true); window->close();
    }
    /** @brief Exercise keyboard-only journeys, accessible names and readable labels with explicit light/dark high-contrast palettes. */
    void keyboardAndAccessibility()
    {
        for (bool dark : {false, true}) {
            QColor background = dark ? Qt::black : Qt::white, foreground = dark ? Qt::white : Qt::black;
            DeviceService service(QString::fromUtf8(DISKSAN_PROCESS_FIXTURE), true);
            QQmlApplicationEngine engine; QSignalSpy warnings(&engine, &QQmlEngine::warnings);
            auto *window = load(engine, service); QVERIFY(window); QVERIFY(QTest::qWaitForWindowExposed(window));
            window->requestActivate(); QVERIFY(QTest::qWaitForWindowActive(window));
            for (const char *role : {"palette.window", "palette.base", "palette.button"})
                QVERIFY(QQmlProperty::write(window, QString::fromLatin1(role), background));
            for (const char *role : {"palette.windowText", "palette.text", "palette.buttonText"})
                QVERIFY(QQmlProperty::write(window, QString::fromLatin1(role), foreground));
            keyboardActivate(window, "refreshButton");
            QTRY_VERIFY_WITH_TIMEOUT(!service.busy() && service.devices().size() == 3, 5000);
            keyboardActivate(window, "deviceRow0"); QTRY_VERIFY_WITH_TIMEOUT(service.fresh() && !service.busy(), 5000);
            auto focusName = [window] {
                auto *item = window->activeFocusItem();
                return item ? item->objectName() : QString{};
            };
            window->requestActivate(); QVERIFY(QTest::qWaitForWindowActive(window));
            QCOMPARE(focusName(), QStringLiteral("deviceRow0"));
            QTest::keyClick(window, Qt::Key_Down);
            QTRY_COMPARE_WITH_TIMEOUT(focusName(), QStringLiteral("deviceRow1"), 3000);
            QTest::keyClick(window, Qt::Key_Up);
            QTRY_COMPARE_WITH_TIMEOUT(focusName(), QStringLiteral("deviceRow0"), 3000);
            keyboardActivate(window, "navigation1"); QCOMPARE(window->property("page").toInt(), 1);
            keyboardActivate(window, "navigation2"); QCOMPARE(window->property("page").toInt(), 2);
            keyboardActivate(window, "planButton"); QTRY_VERIFY_WITH_TIMEOUT(!service.busy() && !service.plan().isEmpty(), 5000);
            keyboardActivate(window, "simulationButton"); QTRY_VERIFY_WITH_TIMEOUT(!service.busy() && !service.simulation().isEmpty(), 5000);
            auto *banner = find(window->contentItem(), "simulationBanner"); QVERIFY(banner);
            auto *accessible = QAccessible::queryAccessibleInterface(banner); QVERIFY(accessible);
            QCOMPARE(accessible->role(), QAccessible::StaticText);
            QVERIFY(accessible->text(QAccessible::Name).contains("DOES NOT ERASE DATA"));
            QCOMPARE(banner->property("color").value<QColor>(), foreground);
            QCOMPARE(window->color(), background);
            keyboardActivate(window, "navigation3"); QCOMPARE(window->property("page").toInt(), 3);
            QCOMPARE(warnings.size(), 0);
            service.shutdown(); window->setProperty("closingReady", true); window->close();
        }
    }
    /** @brief Replace an earlier successful selected view with a redacted error after hostile or denied responses. */
    void selectedFailure()
    {
        for (const QByteArray &mode : {QByteArray("malformed"), QByteArray("denied"), QByteArray("stale")}) {
            qputenv("DISKSAN_GUI_TEST_MODE", "normal");
            DeviceService service(QString::fromUtf8(DISKSAN_PROCESS_FIXTURE), true);
            QQmlApplicationEngine engine; auto *window = load(engine, service); QVERIFY(window);
            QVERIFY(QTest::qWaitForWindowExposed(window)); click(window, "refreshButton");
            QTRY_VERIFY_WITH_TIMEOUT(!service.busy() && service.devices().size() == 3, 5000);
            click(window, "deviceRow0"); QTRY_VERIFY_WITH_TIMEOUT(service.fresh() && !service.busy(), 5000);
            click(window, "navigation2"); qputenv("DISKSAN_GUI_TEST_MODE", mode);
            click(window, "simulationButton"); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 5000);
            QCOMPARE(service.state(), "Error"); QVERIFY(!service.fresh()); QVERIFY(service.simulation().isEmpty());
            QVERIFY(service.assessment().isEmpty()); QVERIFY(!find(window->contentItem(), "planButton")->isEnabled());
            QVERIFY(find(window->contentItem(), "simulationBanner")->isVisible());
            QCOMPARE(find(window->contentItem(), "statusState")->property("text").toString(), "Error");
            QVERIFY(!find(window->contentItem(), "statusMessage")->property("text").toString().contains("PRIVATE-IDENTIFIER"));
            service.shutdown(); window->setProperty("closingReady", true); window->close();
        }
    }
    /** @brief Launch the pinned native CLI through the actual UI for read-only inventory and assessment, without saving private metadata. */
    void nativeInventory()
    {
        DeviceService service(QString::fromUtf8(DISKSAN_NATIVE_CLI));
        QQmlApplicationEngine engine; auto *window = load(engine, service); QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowExposed(window));
        click(window, "refreshButton"); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 20000);
        QVERIFY2(service.state() == "Ready" || service.state() == "Empty", "Native read-only inventory did not complete.");
        int index = -1;
        for (qsizetype i = 0; i < service.devices().size(); ++i)
            if (!service.devices()[i].toMap().value("path").toString().isEmpty()) { index = static_cast<int>(i); break; }
        if (index < 0) {
            service.shutdown(); window->setProperty("closingReady", true); window->close();
            QTest::qSkip("No usable native selector observed.", __FILE__, __LINE__); return;
        }
        service.selectDevice(index); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 20000);
        QVERIFY2(service.fresh(), "Native selected observation failed or became stale; no device data is printed.");
        QVERIFY(!service.information().isEmpty()); QVERIFY(!service.assessment().isEmpty()); QVERIFY(!service.capabilities().isEmpty());
        QVERIFY(!service.capabilities().contains("probe"));
        service.generatePlan(); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 20000);
        QVERIFY(service.fresh()); QVERIFY(!service.plan().value("erase_authorized").toBool());
        service.previewSimulation(); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 20000);
        QVERIFY(service.fresh()); QVERIFY(service.simulation().value("simulation_result").toString() == "non_executable_model_only");
        service.shutdown(); window->setProperty("closingReady", true); window->close();
    }
};
QTEST_MAIN(DesktopTests)
#include "test_desktop_ui.moc"
