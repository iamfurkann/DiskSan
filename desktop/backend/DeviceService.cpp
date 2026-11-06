/** @file
 * @brief Run a pinned CLI with fixed argv, bounded channels, generation checks and asynchronous cancellation.
 */
#include "DeviceService.h"
#include "BoundedJson.h"
#include <QFileInfo>
#include <QDir>
#include <limits>

namespace disksan::desktop {
/** @brief Keep a missing absolute construction-time location retryable without adding PATH lookup or a runtime path setter. */
static QString pinnedExecutable(const QString &path)
{
    QFileInfo info(path);
    if (!info.isAbsolute()) return {};
    QString canonical = info.canonicalFilePath();
    if (!canonical.isEmpty()) return canonical;
    QString parent = QFileInfo(info.absolutePath()).canonicalFilePath();
    return parent.isEmpty() ? QDir::cleanPath(info.absoluteFilePath()) : QDir(parent).filePath(info.fileName());
}
/** @brief Pin only an absolute executable and connect one process to bounded asynchronous handlers. */
DeviceService::DeviceService(const QString &executable, bool demo, QObject *parent, int timeoutMs)
    : QObject(parent), executable_(pinnedExecutable(executable)),
      demo_(demo), timeoutMs_(qBound(10, timeoutMs, 15000))
{
    process_.setProcessChannelMode(QProcess::SeparateChannels);
    process_.setInputChannelMode(QProcess::ManagedInputChannel);
    deadline_.setSingleShot(true);
    connect(&process_, &QProcess::readyReadStandardOutput, this, [this] { readChannel(QProcess::StandardOutput); });
    connect(&process_, &QProcess::readyReadStandardError, this, [this] { readChannel(QProcess::StandardError); });
    connect(&process_, &QProcess::finished, this, &DeviceService::finished);
    connect(&process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        // Qt still owns its failed-start cleanup while emitting this signal.
        // Launch a replacement only after that cleanup releases the old process channels.
        if (error == QProcess::FailedToStart && (closing_ || runningGeneration_ != generation_)) {
            emit changed();
            if (closing_) emit stopped();
            else QTimer::singleShot(0, this, &DeviceService::startNext);
            return;
        }
        if (runningGeneration_ != generation_) return;
        if (error == QProcess::FailedToStart) fail(QStringLiteral("The configured CLI could not start. Build or reinstall the matching CLI."));
        else if (error == QProcess::ReadError || error == QProcess::WriteError)
            fail(QStringLiteral("CLI communication failed. Refresh before retrying."));
    });
    connect(&deadline_, &QTimer::timeout, this, [this] {
        fail(QStringLiteral("CLI inspection timed out. Results were discarded; refresh before retrying."));
    });
}
/** @brief Remove all observation payloads before another request can make them appear current. */
void DeviceService::clearResults()
{
    information_.clear(); assessment_.clear(); capabilities_.clear(); plan_.clear(); simulation_.clear(); fresh_ = false;
}
/** @brief Invalidate one generation and asynchronously retire its single child before replacement. */
void DeviceService::invalidate()
{
    deadline_.stop(); pending_.clear(); clearResults(); stdout_.clear(); stderr_.clear();
    if (generation_ == std::numeric_limits<quint64>::max()) closing_ = true;
    else ++generation_;
    if (process_.state() != QProcess::NotRunning) {
        process_.closeReadChannel(QProcess::StandardOutput); process_.closeReadChannel(QProcess::StandardError);
        process_.kill();
    }
}
/** @brief Verify a known CLI version before requesting a new all-device read-only inventory. */
void DeviceService::refresh()
{
    if (closing_) return;
    invalidate(); devices_.clear(); selected_.clear(); versionValid_ = false;
    state_ = QStringLiteral("Loading"); message_ = QStringLiteral("Checking CLI and refreshing read-only inventory...");
    pending_ = {Command::Version, Command::List}; emit changed(); startNext();
}
/** @brief Accept a current inventory index, never a user-provided command or selector string. */
void DeviceService::selectDevice(int index)
{
    if (closing_ || !versionValid_ || index < 0 || index >= devices_.size()) return;
    QVariantMap selected = devices_[index].toMap();
    invalidate(); selected_ = selected;
    if (commandArguments(Command::Info, selected_.value("path").toString()).isEmpty()) {
        fail(QStringLiteral("This record has no usable native selector. Refresh or inspect it in the CLI.")); return;
    }
    state_ = QStringLiteral("Loading"); message_ = QStringLiteral("Reading device information, assessment and metadata-only capabilities...");
    pending_ = {Command::Info, Command::Assess, Command::Capabilities}; emit changed(); startNext();
}
/** @brief Request one fresh inspection-only response after invalidating the previous selected results. */
void DeviceService::inspect(Command command)
{
    if (closing_ || busy() || !fresh_ || selected_.isEmpty()) return;
    invalidate(); state_ = QStringLiteral("Loading");
    message_ = QStringLiteral("Generating a non-executable inspection model. No storage operation will run.");
    pending_ = {command}; emit changed(); startNext();
}
/** @brief Generate hypothetical prerequisites through the fixed plan argv. */
void DeviceService::generatePlan() { inspect(Command::Plan); }
/** @brief Generate modeled steps through the fixed simulate argv. */
void DeviceService::previewSimulation() { inspect(Command::Simulate); }
/** @brief Cancel current observations and require a fresh selection before reusing their context. */
void DeviceService::cancel()
{
    if (closing_) return;
    invalidate(); state_ = QStringLiteral("Stale"); message_ = QStringLiteral("Inspection canceled. Select a device again or refresh."); emit changed();
}
/** @brief Retire the child asynchronously and notify the app when it can close without a blocking process destructor. */
void DeviceService::shutdown()
{
    if (closing_) return;
    closing_ = true; invalidate(); devices_.clear(); selected_.clear();
    state_ = QStringLiteral("Closing"); message_ = QStringLiteral("Closing read-only inspection..."); emit changed();
    if (process_.state() == QProcess::NotRunning) emit stopped();
}
/** @brief Keep child failures separate from valid blocked/review outcomes and discard all selected payloads. */
void DeviceService::fail(const QString &message)
{
    invalidate(); state_ = QStringLiteral("Error"); message_ = message; emit changed();
}
/** @brief Start only one enum-derived argv on the pinned executable with closed stdin and a finite deadline. */
void DeviceService::startNext()
{
    if (closing_ || process_.state() != QProcess::NotRunning || pending_.isEmpty()) return;
    QFileInfo binary(executable_);
    if (executable_.isEmpty() || !binary.isFile() || !binary.isExecutable() || binary.canonicalFilePath() != executable_) {
        fail(QStringLiteral("Matching CLI is unavailable. Build the CLI beside this desktop build, then refresh.")); return;
    }
    current_ = pending_.takeFirst(); runningGeneration_ = generation_;
    QString selector = current_ == Command::Version || current_ == Command::List ? QString{} : selected_.value("path").toString();
    auto arguments = commandArguments(current_, selector);
    if (arguments.isEmpty()) { fail(QStringLiteral("Invalid inspection request. Refresh before retrying.")); return; }
    stdout_.clear(); stderr_.clear();
    process_.setWorkingDirectory(binary.absolutePath());
    process_.setProgram(executable_); process_.setArguments(arguments);
    deadline_.start(timeoutMs_);
    process_.start(QIODevice::ReadOnly); process_.closeWriteChannel(); emit changed();
}
/** @brief Bound both process channels before buffering and never surface raw stderr as UI text. */
void DeviceService::readChannel(QProcess::ProcessChannel channel)
{
    if (runningGeneration_ != generation_) return;
    process_.setReadChannel(channel);
    QByteArray &buffer = channel == QProcess::StandardOutput ? stdout_ : stderr_;
    qsizetype limit = channel == QProcess::StandardOutput ? JsonByteLimit : 32768;
    QByteArray chunk = process_.read(limit - buffer.size() + 1);
    if (chunk.size() > limit - buffer.size()) { fail(QStringLiteral("CLI output exceeded its limit. Results were discarded.")); return; }
    buffer += chunk;
}
/** @brief Reject stale, crashed, nonzero or malformed responses before advancing the fixed request sequence. */
void DeviceService::finished(int exitCode, QProcess::ExitStatus status)
{
    deadline_.stop();
    if (closing_) { stdout_.clear(); stderr_.clear(); emit changed(); emit stopped(); return; }
    if (runningGeneration_ != generation_) { startNext(); emit changed(); return; }
    readChannel(QProcess::StandardOutput); readChannel(QProcess::StandardError);
    if (runningGeneration_ != generation_) { emit changed(); return; }
    if (status != QProcess::NormalExit || exitCode != 0) {
        fail(exitCode == 4 ? QStringLiteral("Permission denied during read-only inspection. No elevation is requested.") :
             exitCode == 3 ? QStringLiteral("Device changed, disappeared or is not a valid target. Refresh the inventory.") :
             QStringLiteral("CLI inspection failed. No results were accepted; refresh before retrying.")); return;
    }
    auto reply = validateReply(current_, stdout_, demo_); stdout_.clear(); stderr_.clear();
    if (!reply.ok) { fail(reply.error); return; }
    if (current_ == Command::Version) versionValid_ = true;
    else if (current_ == Command::List) devices_ = reply.data.value("devices").toList();
    else {
        QVariantMap observed = current_ == Command::Info ? reply.data.value("devices").toList().first().toMap() : reply.data.value("device").toMap();
        if (!sameDevice(selected_, observed)) {
            fail(QStringLiteral("Selected device identity or relevant state changed. Refresh before retrying.")); return;
        }
        switch (current_) {
            case Command::Info: information_ = reply.data; break;
            case Command::Assess: assessment_ = reply.data; break;
            case Command::Capabilities: capabilities_ = reply.data; break;
            case Command::Plan: plan_ = reply.data; break;
            case Command::Simulate: simulation_ = reply.data; break;
            default: break;
        }
    }
    if (!pending_.isEmpty()) { startNext(); return; }
    fresh_ = !selected_.isEmpty();
    state_ = devices_.isEmpty() ? QStringLiteral("Empty") : QStringLiteral("Ready");
    message_ = devices_.isEmpty() ? QStringLiteral("No devices were reported. Empty inventory does not establish device safety.") :
        fresh_ ? QStringLiteral("Observation complete. Identity is non-atomic and has no future guarantee.") :
                 QStringLiteral("Inventory refreshed. Select a device to inspect observed evidence.");
    emit changed();
}
}
