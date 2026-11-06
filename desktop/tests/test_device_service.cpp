/** @file
 * @brief Exercise the real QProcess bridge against pinned synthetic executables, including cancellation and hostile output.
 */
#include "DeviceService.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtTest>

using namespace disksan::desktop;
/** @brief Validate observable process lifecycle and state transitions without accessing storage devices. */
class ServiceTests final : public QObject {
    Q_OBJECT
private slots:
    /** @brief Reset test-only child behavior before each independent scenario. */
    void init() { qputenv("DISKSAN_GUI_TEST_MODE", "normal"); qunsetenv("DISKSAN_GUI_TEST_LOG"); }
    /** @brief Remove process-test environment overrides when a scenario finishes. */
    void cleanup() { qunsetenv("DISKSAN_GUI_TEST_MODE"); qunsetenv("DISKSAN_GUI_TEST_LOG"); }
    /** @brief Drive inventory, selection, plan and simulation with exact fixed argv and synthetic provenance. */
    void normalFlow()
    {
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        QString path = temporary.filePath("argv.jsonl"); qputenv("DISKSAN_GUI_TEST_LOG", path.toUtf8());
        DeviceService service(QString::fromUtf8(DISKSAN_PROCESS_FIXTURE), true);
        QCOMPARE(service.state(), "Disconnected"); QVERIFY(service.devices().isEmpty()); QVERIFY(!service.fresh());
        service.refresh(); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 5000);
        QVERIFY2(service.state() == "Ready", qPrintable(service.message())); QCOMPARE(service.devices().size(), 3);
        service.selectDevice(0); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 5000);
        QVERIFY(service.fresh()); QVERIFY(!service.information().isEmpty()); QVERIFY(!service.assessment().isEmpty());
        QVERIFY(!service.capabilities().isEmpty());
        QCOMPARE(service.selected().value("size_bytes").toString(), "18446744073709551615");
        service.generatePlan(); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 5000);
        QCOMPARE(service.plan().value("operation").toString(), "plan"); QVERIFY(service.fresh());
        service.previewSimulation(); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 5000);
        QCOMPARE(service.simulation().value("simulation_result").toString(), "non_executable_model_only");
        QCOMPARE(service.simulation().value("steps").toList().size(), 10);
        service.selectDevice(2); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 5000);
        QVERIFY(service.fresh()); QCOMPARE(service.selected().value("path").toString(), "--probe");
        QFile file(path); QVERIFY(file.open(QIODevice::ReadOnly));
        auto lines = file.readAll().trimmed().split('\n'); QCOMPARE(lines.size(), 10);
        for (const auto &line : lines) {
            auto args = QJsonDocument::fromJson(line).array();
            QVERIFY(args[0].toString() != "probe");
            if (args.size() == 4) { QCOMPARE(args[1].toString(), "--json"); QCOMPARE(args[2].toString(), "--"); }
        }
        QSignalSpy stopped(&service, &DeviceService::stopped); service.shutdown(); QCOMPARE(stopped.size(), 1);
    }
    /** @brief Report missing CLI and wrong version without fabricating a fallback inventory. */
    void unavailable()
    {
        DeviceService missing(QStringLiteral("relative-program"));
        missing.refresh(); QCOMPARE(missing.state(), "Error"); QVERIFY(missing.devices().isEmpty()); QVERIFY(!missing.busy());
        qputenv("DISKSAN_GUI_TEST_MODE", "version");
        DeviceService wrong(QString::fromUtf8(DISKSAN_PROCESS_FIXTURE), true);
        wrong.refresh(); QTRY_VERIFY_WITH_TIMEOUT(!wrong.busy(), 5000);
        QCOMPARE(wrong.state(), "Error"); QVERIFY(wrong.devices().isEmpty());
    }
    /** @brief Retry the same pinned missing absolute location after installation, without accepting a different executable path. */
    void installedAfterStart()
    {
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
#ifdef Q_OS_WIN
        QString path = temporary.filePath("known-cli.exe");
#else
        QString path = temporary.filePath("known-cli");
#endif
        DeviceService service(path, true);
        service.refresh(); QCOMPARE(service.state(), "Error"); QVERIFY(!service.busy());
        QVERIFY(QFile::copy(QString::fromUtf8(DISKSAN_PROCESS_FIXTURE), path));
        service.refresh(); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 5000);
        QCOMPARE(service.state(), "Ready"); QCOMPARE(service.devices().size(), 3);
    }
    /** @brief Retire a child whose pinned file exists but cannot launch, including during refresh or shutdown. */
    void invalidExecutableLifecycle()
    {
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        QFile file(temporary.filePath("invalid-cli.exe"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.write(QByteArray(32, '\0')) == 32);
        file.close();
        QVERIFY(file.setPermissions(QFileDevice::ReadOwner | QFileDevice::ExeOwner));
        DeviceService service(file.fileName(), true);
        service.refresh(); service.refresh();
        QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 5000);
        QCOMPARE(service.state(), "Error"); QVERIFY(service.devices().isEmpty());
        QVERIFY2(service.message().contains("could not start"), qPrintable(service.message()));
        QSignalSpy stopped(&service, &DeviceService::stopped);
        service.refresh(); service.shutdown();
        QTRY_COMPARE_WITH_TIMEOUT(stopped.size(), 1, 5000);
        QCOMPARE(service.state(), "Closing"); QVERIFY(!service.busy());
    }
    /** @brief Distinguish an empty valid inventory from an assessment that found no blockers. */
    void emptyInventory()
    {
        qputenv("DISKSAN_GUI_TEST_MODE", "empty");
        DeviceService service(QString::fromUtf8(DISKSAN_PROCESS_FIXTURE), true);
        service.refresh(); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 5000);
        QCOMPARE(service.state(), "Empty"); QVERIFY(service.devices().isEmpty()); QVERIFY(!service.fresh());
    }
    /** @brief Discard every payload after malformed, denied, changed, oversized or nonzero responses. */
    void failures()
    {
        for (const QByteArray &mode : {QByteArray("malformed"),QByteArray("denied"),QByteArray("stale"),
                                      QByteArray("stdout"),QByteArray("stderr"),QByteArray("nonzero")}) {
            qputenv("DISKSAN_GUI_TEST_MODE", mode);
            DeviceService service(QString::fromUtf8(DISKSAN_PROCESS_FIXTURE), true);
            service.refresh(); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 5000);
            QCOMPARE(service.devices().size(), 3);
            service.selectDevice(0); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 5000);
            QCOMPARE(service.state(), "Error"); QVERIFY(!service.fresh()); QVERIFY(service.information().isEmpty());
            QVERIFY(service.assessment().isEmpty()); QVERIFY(service.plan().isEmpty()); QVERIFY(service.simulation().isEmpty());
            QVERIFY(!service.message().contains("PRIVATE-IDENTIFIER"));
            service.generatePlan(); QVERIFY(!service.busy());
        }
    }
    /** @brief Kill a slow child at its deadline while the event loop remains responsive. */
    void timeout()
    {
        qputenv("DISKSAN_GUI_TEST_MODE", "timeout");
        DeviceService service(QString::fromUtf8(DISKSAN_PROCESS_FIXTURE), true, nullptr, 500);
        service.refresh(); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 5000);
        QCOMPARE(service.devices().size(), 3);
        service.selectDevice(0); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 5000);
        QCOMPARE(service.state(), "Error"); QVERIFY(service.message().contains("timed out")); QVERIFY(!service.fresh());
    }
    /** @brief Supersede a slow selection with another index and reject every old-generation response. */
    void supersededSelection()
    {
        qputenv("DISKSAN_GUI_TEST_MODE", "delay-first");
        DeviceService service(QString::fromUtf8(DISKSAN_PROCESS_FIXTURE), true);
        service.refresh(); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 5000);
        service.selectDevice(0); QVERIFY(service.busy());
        service.selectDevice(1); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 5000);
        QVERIFY(service.fresh()); QCOMPARE(service.selected().value("path").toString(), "second");
        QCOMPARE(service.assessment().value("device").toMap().value("path").toString(), "second");
        service.selectDevice(0); service.refresh(); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 5000);
        QVERIFY(service.selected().isEmpty()); QVERIFY(service.assessment().isEmpty()); QCOMPARE(service.devices().size(), 3);
    }
    /** @brief Cancel and close during live work without starting replacement children or retaining success state. */
    void cancellationAndShutdown()
    {
        qputenv("DISKSAN_GUI_TEST_MODE", "timeout");
        DeviceService service(QString::fromUtf8(DISKSAN_PROCESS_FIXTURE), true);
        service.refresh(); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 5000);
        service.selectDevice(0); service.cancel(); QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 5000);
        QCOMPARE(service.state(), "Stale"); QVERIFY(!service.fresh()); QVERIFY(service.assessment().isEmpty());
        service.selectDevice(0);
        QSignalSpy stopped(&service, &DeviceService::stopped); service.shutdown();
        QTRY_COMPARE_WITH_TIMEOUT(stopped.size(), 1, 5000); QVERIFY(!service.busy());
        service.refresh(); QVERIFY(!service.busy()); QCOMPARE(service.state(), "Closing");
    }
};
QTEST_GUILESS_MAIN(ServiceTests)
#include "test_device_service.moc"
