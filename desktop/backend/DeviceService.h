/** @file
 * @brief Own one asynchronous pinned CLI process and invalidate observations whenever selection or requests change.
 */
#ifndef DISKSAN_DESKTOP_DEVICE_SERVICE_H
#define DISKSAN_DESKTOP_DEVICE_SERVICE_H
#include "CliContract.h"
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QVariantList>

namespace disksan::desktop {
/** @brief Unprivileged desktop state; only named read-only actions are callable from QML. */
class DeviceService final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList devices READ devices NOTIFY changed)
    Q_PROPERTY(QVariantMap selected READ selected NOTIFY changed)
    Q_PROPERTY(QVariantMap information READ information NOTIFY changed)
    Q_PROPERTY(QVariantMap assessment READ assessment NOTIFY changed)
    Q_PROPERTY(QVariantMap capabilities READ capabilities NOTIFY changed)
    Q_PROPERTY(QVariantMap plan READ plan NOTIFY changed)
    Q_PROPERTY(QVariantMap simulation READ simulation NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString state READ state NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool fresh READ fresh NOTIFY changed)
    Q_PROPERTY(bool demo READ demo CONSTANT)
public:
    /**
     * @brief Pin one absolute executable supplied by trusted application construction, never by QML or metadata.
     * @param[in] executable Absolute known CLI file; relative, missing or non-executable files leave the session disconnected.
     * @param[in] demo Explicit fixture-session flag, permanently exposed to the UI as DEMO DATA.
     * @param[in] parent Optional QObject owner; call shutdown and wait for stopped before destroying a running service.
     * @param[in] timeoutMs Bounded deadline, normally 15000 ms; shorter values support deterministic process tests.
     */
    explicit DeviceService(const QString &executable, bool demo = false, QObject *parent = nullptr, int timeoutMs = 15000);
    /** @brief Expose only validated inventory data. @return Implicitly shared owned list, empty before a successful refresh. */
    QVariantList devices() const { return devices_; }
    /** @brief Expose the selected validated inventory record. @return Owned shared map, empty without a selection. */
    QVariantMap selected() const { return selected_; }
    /** @brief Expose validated info output. @return Owned shared map, empty when invalidated. */
    QVariantMap information() const { return information_; }
    /** @brief Expose observed assessment output. @return Owned shared map including blockers/unknowns, or empty when invalidated. */
    QVariantMap assessment() const { return assessment_; }
    /** @brief Expose metadata-only capability evidence. @return Owned shared map without a probe receipt, or empty. */
    QVariantMap capabilities() const { return capabilities_; }
    /** @brief Expose non-executable prerequisites. @return Owned shared plan map, empty when invalidated. */
    QVariantMap plan() const { return plan_; }
    /** @brief Expose modeled steps only. @return Owned shared simulation map, empty when invalidated; never an operation result. */
    QVariantMap simulation() const { return simulation_; }
    /** @brief Expose redacted status text. @return Owned status string, excluding raw stderr and device identifiers. */
    QString message() const { return message_; }
    /** @brief Expose the presentation state. @return Disconnected, Loading, Ready, Empty, Error, Stale or Closing. */
    QString state() const { return state_; }
    /** @brief Report pending work. @return True while the sole process or its fixed request sequence remains pending. */
    bool busy() const { return process_.state() != QProcess::NotRunning || !pending_.isEmpty(); }
    /** @brief Describe response freshness within this UI generation. @return True for current selected responses, never global safety. */
    bool fresh() const { return fresh_; }
    /** @brief Distinguish synthetic sessions. @return True only for an explicitly constructed fixture session. */
    bool demo() const { return demo_; }
    /** @brief Clear prior data, check the pinned CLI version, then request a new read-only inventory. */
    Q_INVOKABLE void refresh();
    /**
     * @brief Select a validated inventory index and request info, assessment and metadata capabilities.
     * @param[in] index Current zero-based inventory index; out-of-range indices do nothing. No selector string is accepted.
     */
    Q_INVOKABLE void selectDevice(int index);
    /** @brief Request fresh hypothetical prerequisites for the selected observation; no method selection exists. */
    Q_INVOKABLE void generatePlan();
    /** @brief Request fresh modeled steps for the selected observation without probing or executing storage operations. */
    Q_INVOKABLE void previewSimulation();
    /** @brief Invalidate pending observations and kill the child asynchronously without blocking the UI thread. */
    Q_INVOKABLE void cancel();
    /** @brief Cancel pending requests and emit stopped only after the sole child has actually terminated. */
    Q_INVOKABLE void shutdown();
signals:
    /** @brief Notify all UI properties after a coherent state transition. */
    void changed();
    /** @brief Signal safe service destruction after asynchronous child cleanup completes. */
    void stopped();
private:
    /** @brief Clear all selected observation payloads, retaining no stale success results. */
    void clearResults();
    /** @brief Retire the current generation, close its channels and kill its child before any replacement starts. */
    void invalidate();
    /** @brief Start the next fixed operation only when the previous process has stopped. */
    void startNext();
    /** @brief Drain one channel through its independent output cap and reject oversized output. */
    void readChannel(QProcess::ProcessChannel channel);
    /** @brief Accept only successful current-generation complete responses and verify selected identity. */
    void finished(int exitCode, QProcess::ExitStatus status);
    /** @brief Clear payloads and retire a failed request without exposing raw child diagnostics. */
    void fail(const QString &message);
    /** @brief Queue a single inspection-only operation when a current selection exists. */
    void inspect(Command command);

    const QString executable_; /**< @brief Canonical trusted construction-time executable path; never changed by QML. */
    const bool demo_; /**< @brief Permanent session provenance. */
    const int timeoutMs_; /**< @brief Validated finite per-process deadline. */
    QProcess process_; /**< @brief Sole child process; no concurrent replacement can accumulate. */
    QTimer deadline_; /**< @brief Async deadline; no waitFor calls occur on the UI thread. */
    QList<Command> pending_; /**< @brief Fixed bounded sequence, at most three operations. */
    Command current_ = Command::Version; /**< @brief Operation corresponding to the current response. */
    quint64 generation_ = 0, runningGeneration_ = 0; /**< @brief Invalidation counters, checked before each response. */
    QByteArray stdout_, stderr_; /**< @brief Bounded private child output, cleared after completion/error. */
    QVariantList devices_; /**< @brief Validated current inventory only. */
    QVariantMap selected_, information_, assessment_, capabilities_, plan_, simulation_; /**< @brief Per-selection typed snapshots. */
    QString message_ = QStringLiteral("Disconnected. Refresh to inspect devices."); /**< @brief Static or count-only status message. */
    QString state_ = QStringLiteral("Disconnected"); /**< @brief Explicit presentation state. */
    bool fresh_ = false, versionValid_ = false, closing_ = false; /**< @brief UI lifecycle state, never an authorization bit. */
};
}
#endif
