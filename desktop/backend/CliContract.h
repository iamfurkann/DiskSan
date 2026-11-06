/** @file
 * @brief Define the desktop's fixed CLI vocabulary and validate complete typed responses before presentation.
 */
#ifndef DISKSAN_DESKTOP_CLI_CONTRACT_H
#define DISKSAN_DESKTOP_CLI_CONTRACT_H
#include <QByteArray>
#include <QStringList>
#include <QVariantMap>

namespace disksan::desktop {
/** @brief Every allowed process operation; there is no probe, execution, shell or arbitrary command member. */
enum class Command { Version, List, Info, Assess, Capabilities, Plan, Simulate };
/** @brief Own only a complete checked response or a static redacted error. */
struct CliReply {
    bool ok = false; /**< @brief Complete schema and invariant checks passed. */
    QVariantMap data; /**< @brief Typed display data; uint64 values remain exact decimal strings. */
    QString error; /**< @brief Generic failure explanation, never raw stdout or identifiers. */
};
/**
 * @brief Produce one fixed argument vector with the selector passed as a single opaque value after literal --.
 * @param[in] command Enumerated allowed operation; invalid values are rejected.
 * @param[in] selector Borrowed current-inventory path for targeted operations, empty for list/version.
 * @return Owned arguments, or an empty list for an invalid operation/selector combination.
 */
QStringList commandArguments(Command command, const QString &selector = {});
/**
 * @brief Validate bounded CLI bytes against the exact desktop build version and requested operation.
 * @param[in] command The operation actually requested, never inferred from child output.
 * @param[in] bytes Borrowed complete successful-process stdout; nonzero/crashed children must be rejected by the caller first.
 * @param[in] synthetic True only in an explicitly labeled fixture session; native sessions reject synthetic plans.
 * @return Complete owned display document or an empty result with static error text.
 * @note This checks structure and invariants, not binary authenticity or device safety; no response authorizes anything.
 */
CliReply validateReply(Command command, const QByteArray &bytes, bool synthetic = false);
/**
 * @brief Match observed identity and relevant state against the selected inventory record before accepting a later response.
 * @param[in] selected Borrowed validated inventory record; requires a nonempty path.
 * @param[in] observed Borrowed validated record from a fresh info/assessment/plan response.
 * @return True only when compared fields still match; this is not atomic identity or future authorization.
 */
bool sameDevice(const QVariantMap &selected, const QVariantMap &observed);
}
#endif
