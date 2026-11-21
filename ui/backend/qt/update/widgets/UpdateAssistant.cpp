#include "ui/backend/qt/update/widgets/UpdateAssistant.hpp"
#include "ui/backend/qt/update/IssueReport.hpp"
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <utility>

namespace ui::backend::qt::update
{
    namespace
    {
        const QString skippedVersionKey{ "updates/skippedVersion" };
        const QString automaticKey{ "updates/checkAtStartup" };

        void RemoveStaleDownloads(const QString& directory)
        {
            const auto cutoff = QDateTime::currentDateTimeUtc().addDays(-1);
            QDirIterator files{ directory, QDir::Files };
            while (files.hasNext())
                if (const auto file = files.nextFileInfo(); file.lastModified().toUTC() < cutoff)
                    QFile::remove(file.filePath());
        }
    }

    UpdateAssistant::UpdateAssistant(const UpdateProduct& product, QWidget& window, UpdateSession::CommandFactory commandFor)
        : QObject(&window)
        , product(product)
        , window(window)
        , checker(ReleaseVersion::Parse(product.version.toStdString()).value_or(ReleaseVersion{}), product.feedUrl, product.userAgent)
        , session(DownloadDirectory(), product.userAgent, std::move(commandFor))
        , banner(new UpdateBanner(product, &window))
    {
        ConnectChecker();
        ConnectSession();
        RemoveStaleDownloads(DownloadDirectory());
        connect(&automaticTimer, &QTimer::timeout, this, [this]()
            {
                if (Automatic())
                    Check(false);
            });
    }

    UpdateBanner& UpdateAssistant::Banner()
    {
        return *banner;
    }

    const UpdateState& UpdateAssistant::State() const
    {
        return session.State();
    }

    void UpdateAssistant::Check(bool userInitiated)
    {
        userRequested = userRequested || userInitiated;
        if (userInitiated)
            emit Checking();
        checker.Check();
    }

    void UpdateAssistant::StartAutomaticChecks()
    {
        automaticTimer.start(automaticInterval);
        if (Automatic())
            Check(false);
    }

    bool UpdateAssistant::Automatic() const
    {
        return QSettings{}.value(automaticKey, true).toBool();
    }

    void UpdateAssistant::SetAutomatic(bool automatic) const
    {
        QSettings{}.setValue(automaticKey, automatic);
    }

    void UpdateAssistant::SetBlocked(bool blocked)
    {
        session.SetBlocked(blocked);
    }

    std::optional<InstallCommand> UpdateAssistant::DefaultCommand(const QString& installerPath)
    {
        return DefaultUpdateCommand(CurrentInstallerPlatform(), installerPath, QCoreApplication::applicationFilePath(), QCoreApplication::applicationPid());
    }

    QString UpdateAssistant::DownloadDirectory()
    {
        return QDir{ QStandardPaths::writableLocation(QStandardPaths::CacheLocation) }.filePath("updates");
    }

    void UpdateAssistant::ConnectChecker()
    {
        connect(&checker, &UpdateChecker::UpdateAvailable, this, &UpdateAssistant::OnAvailable);
        connect(&checker, &UpdateChecker::UpToDate, this, [this](const ReleaseInfo& latest)
            {
                emit Report(IssueSeverity::info, QString("Up to date (latest release %1)").arg(latest.tag));
                if (std::exchange(userRequested, false))
                    QMessageBox::information(&window, "Check for Updates", QString("You are running the latest version (%1).").arg(product.version));
            });
        connect(&checker, &UpdateChecker::CheckFailed, this, [this](const QString& reason)
            {
                emit Report(IssueSeverity::warning, QString("Update check failed: %1").arg(reason));
                if (std::exchange(userRequested, false))
                    QMessageBox::warning(&window, "Check for Updates",
                        QString("Could not check for updates: %1\n\nReleases are also listed at %2").arg(reason, product.releasesUrl.toString()));
            });
    }

    void UpdateAssistant::ConnectSession()
    {
        connect(&session, &UpdateSession::StateChanged, this, &UpdateAssistant::OnStateChanged);
        connect(&session, &UpdateSession::RestartRequested, this, [this]()
            {
                emit Report(IssueSeverity::info, QString("Closing so the installer can update to %1").arg(session.State().release->tag));
                emit RestartRequested();
            });
        connect(&session, &UpdateSession::Relaunch, this, [this]()
            {
                emit Report(IssueSeverity::info, QString("Update %1 installed; restarting").arg(session.State().release->tag));
                emit RelaunchRequested();
            });
        connect(banner, &UpdateBanner::InstallRequested, this, &UpdateAssistant::RequestInstall);
        connect(banner, &UpdateBanner::CancelRequested, &session, &UpdateSession::Cancel);
        connect(banner, &UpdateBanner::DismissRequested, &session, &UpdateSession::Dismiss);
        connect(banner, &UpdateBanner::SkipRequested, this, &UpdateAssistant::Skip);
    }

    void UpdateAssistant::OnAvailable(const ReleaseInfo& release)
    {
        const auto requested = std::exchange(userRequested, false);
        if (!requested && QSettings{}.value(skippedVersionKey).toString() == release.tag)
        {
            emit Report(IssueSeverity::info, QString("Update available: %1 (skipped)").arg(release.tag));
            return;
        }

        emit Report(IssueSeverity::info, QString("Update available: %1").arg(release.tag));
        session.Offer(release);
    }

    void UpdateAssistant::OnStateChanged(const UpdateState& state)
    {
        banner->Render(state);

        if (std::exchange(lastPhase, state.phase) == state.phase || !state.release)
            return;

        if (const auto report = PhaseReport(state))
            emit Report(report->severity, report->message);
    }

    void UpdateAssistant::RequestInstall()
    {
        const auto& state = session.State();
        if (state.phase == UpdatePhase::available || state.phase == UpdatePhase::failed)
        {
            QMessageBox confirm{ &window };
            confirm.setIcon(QMessageBox::Question);
            confirm.setWindowTitle("Install Update");
            confirm.setText(QString("Install %1 %2?").arg(product.displayName, QString::fromStdString(state.release->version.ToString())));
            confirm.setInformativeText(InstallExplanation(product, state.blocked, CurrentInstallerPlatform()));
            auto* install = confirm.addButton("&Install and Restart", QMessageBox::AcceptRole);
            confirm.addButton(QMessageBox::Cancel);
            confirm.setDefaultButton(install);
            confirm.exec();
            if (confirm.clickedButton() != install)
                return;
        }

        session.Install();
    }

    void UpdateAssistant::Skip()
    {
        if (const auto& release = session.State().release)
        {
            QSettings{}.setValue(skippedVersionKey, release->tag);
            emit Report(IssueSeverity::info, QString("Update %1 skipped").arg(release->tag));
        }
        session.Dismiss();
    }

    QString InstallExplanation(const UpdateProduct& product, bool blocked, InstallerPlatform platform)
    {
        auto explanation = platform == InstallerPlatform::windowsX64
                               ? QString("%1 downloads the installer and checks it against the release. It then closes, Windows asks for permission, and the new version starts when the installation is done.").arg(product.shortName)
                               : QString("%1 downloads the package and checks it against the release, then asks for your password to install it and restarts.").arg(product.shortName);
        if (blocked && !product.blockedAction.isEmpty())
            explanation += QString("\n\nInstalling waits until you %1%2.").arg(product.blockedAction.left(1).toLower(), product.blockedAction.mid(1));
        return explanation;
    }

    std::optional<PhaseMessage> PhaseReport(const UpdateState& state)
    {
        if (!state.release)
            return std::nullopt;

        const auto& tag = state.release->tag;
        switch (state.phase)
        {
            case UpdatePhase::downloading:
                return PhaseMessage{ IssueSeverity::info, QString("Downloading update %1 from %2").arg(tag, state.release->installer ? state.release->installer->url.toString() : state.release->pageUrl.toString()) };
            case UpdatePhase::ready:
                return PhaseMessage{ IssueSeverity::info, QString("Update %1 downloaded and verified").arg(tag) };
            case UpdatePhase::installing:
                return PhaseMessage{ IssueSeverity::info, QString("Installing update %1").arg(tag) };
            case UpdatePhase::failed:
                return PhaseMessage{ IssueSeverity::warning, QString("Update to %1 failed: %2").arg(tag, state.failure) };
            default:
                return std::nullopt;
        }
    }

    bool OpenIssueReport(const UpdateProduct& product, const IssueLog& log, QWidget* parent)
    {
        const auto environment = CurrentIssueEnvironment(IssueProduct{ product.displayName, product.shortName }, product.version, log.FilePath());
        const auto url = BuildIssueUrl(product.newIssueUrl, environment, log.RecentLines(60));
        if (QDesktopServices::openUrl(url))
            return true;

        QMessageBox::warning(parent, "Report Issue",
            QString("Could not open a web browser.\n\nPlease report the issue at %1 and attach the log file:\n%2")
                .arg(product.newIssueUrl.toString(), QDir::toNativeSeparators(log.FilePath())));
        return false;
    }
}
