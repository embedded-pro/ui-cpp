#pragma once

#include "ui/backend/qt/update/ReleaseFeed.hpp"
#include <QString>
#include <QStringList>
#include <optional>

namespace ui::backend::qt::update
{
    enum class InstallationKind
    {
        package,
        portable
    };

    struct InstallCommand
    {
        QString program;
        QStringList arguments;
        bool exitsBeforeInstalling{ false };
    };

    InstallationKind DetectInstallation(InstallerPlatform platform, const QString& applicationFilePath);
    QString WindowsInstallDirectory(const QString& applicationFilePath);

    InstallCommand WindowsUpdateCommand(const QString& installerPath, const QString& installDirectory, qint64 processId, const QString& applicationFilePath);
    InstallCommand LinuxUpdateCommand(const QString& elevationProgram, const QString& packageManager, const QString& packagePath);

    std::optional<InstallCommand> DefaultUpdateCommand(InstallerPlatform platform, const QString& installerPath, const QString& applicationFilePath, qint64 processId);
}
