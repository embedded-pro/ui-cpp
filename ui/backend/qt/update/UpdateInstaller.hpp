#pragma once

#include "ui/backend/qt/update/UpdateInstallation.hpp"
#include <QObject>
#include <QProcess>

namespace ui::backend::qt::update
{
    class UpdateInstaller
        : public QObject
    {
        Q_OBJECT

    public:
        explicit UpdateInstaller(QObject* parent = nullptr);

        void Install(const InstallCommand& command);
        bool IsInstalling() const;

    signals:
        void HandedOver();
        void Installed();
        void Failed(const QString& reason);

    private:
        void OnFinished(int exitCode, QProcess::ExitStatus status);

        QProcess process;
    };
}
