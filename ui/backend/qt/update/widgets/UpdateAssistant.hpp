#pragma once

#include "ui/backend/qt/update/IssueLog.hpp"
#include "ui/backend/qt/update/UpdateChecker.hpp"
#include "ui/backend/qt/update/UpdateInstallation.hpp"
#include "ui/backend/qt/update/UpdateSession.hpp"
#include "ui/backend/qt/update/widgets/UpdateBanner.hpp"
#include "ui/backend/qt/update/widgets/UpdateProduct.hpp"
#include <QObject>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <chrono>
#include <optional>

namespace ui::backend::qt::update
{
    class UpdateAssistant
        : public QObject
    {
        Q_OBJECT

    public:
        static constexpr std::chrono::hours automaticInterval{ 24 };

        UpdateAssistant(const UpdateProduct& product, QWidget& window, UpdateSession::CommandFactory commandFor = DefaultCommand);

        UpdateBanner& Banner();
        const UpdateState& State() const;

        void Check(bool userInitiated);
        void StartAutomaticChecks();
        bool Automatic() const;
        void SetAutomatic(bool automatic);
        void SetBlocked(bool blocked);

        static std::optional<InstallCommand> DefaultCommand(const QString& installerPath);
        static QString DownloadDirectory();

    signals:
        void Report(ui::backend::qt::update::IssueSeverity severity, const QString& message);
        void Checking();
        void RestartRequested();
        void RelaunchRequested();

    private:
        void ConnectChecker();
        void ConnectSession();
        void OnAvailable(const ReleaseInfo& release);
        void OnStateChanged(const UpdateState& state);
        void RequestInstall();
        void Skip();
        QString InstallSteps() const;

        UpdateProduct product;
        QWidget& window;
        UpdateChecker checker;
        UpdateSession session;
        UpdateBanner* banner;
        QTimer automaticTimer;
        bool userRequested{ false };
        UpdatePhase lastPhase{ UpdatePhase::idle };
    };

    struct PhaseMessage
    {
        IssueSeverity severity;
        QString message;
    };

    std::optional<PhaseMessage> PhaseReport(const UpdateState& state);
    bool OpenIssueReport(const UpdateProduct& product, const IssueLog& log, QWidget* parent);
}
