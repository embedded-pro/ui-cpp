#pragma once

#include "ui/backend/qt/update/ReleaseFeed.hpp"
#include "ui/backend/qt/update/UpdateDownloader.hpp"
#include "ui/backend/qt/update/UpdateInstaller.hpp"
#include <QObject>
#include <functional>
#include <optional>

namespace ui::backend::qt::update
{
    enum class UpdatePhase
    {
        idle,
        available,
        downloading,
        ready,
        installing,
        failed
    };

    struct UpdateState
    {
        UpdatePhase phase{ UpdatePhase::idle };
        std::optional<ReleaseInfo> release;
        bool automaticInstall{ false };
        bool blocked{ false };
        qint64 received{ 0 };
        qint64 total{ 0 };
        QString failure;
    };

    class UpdateSession
        : public QObject
    {
        Q_OBJECT

    public:
        using CommandFactory = std::function<std::optional<InstallCommand>(const QString& installerPath)>;

        UpdateSession(const QString& downloadDirectory, const QByteArray& userAgent, CommandFactory commandFor, QObject* parent = nullptr);

        void Offer(const ReleaseInfo& release);
        void Install();
        void Cancel();
        void Dismiss();
        void SetBlocked(bool blocked);

        const UpdateState& State() const;

    signals:
        void StateChanged(const ui::backend::qt::update::UpdateState& state);
        void RestartRequested();
        void Relaunch();

    private:
        bool Busy() const;
        QString InstallerPath(const InstallerAsset& asset) const;
        void StartDownload();
        void StartInstall();
        void Fail(const QString& reason);
        void Enter(UpdatePhase phase);

        QString downloadDirectory;
        CommandFactory commandFor;
        UpdateDownloader downloader;
        UpdateInstaller installer;
        UpdateState state;
        QString downloadedPath;
    };
}
