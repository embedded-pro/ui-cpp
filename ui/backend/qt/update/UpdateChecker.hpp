#pragma once

#include "ui/backend/qt/update/ReleaseFeed.hpp"
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>

class QNetworkReply;

namespace ui::backend::qt::update
{
    class UpdateChecker
        : public QObject
    {
        Q_OBJECT

    public:
        UpdateChecker(const ReleaseVersion& current, const QUrl& feedUrl, const QByteArray& userAgent, QObject* parent = nullptr);

        void Check();
        bool IsChecking() const;

    signals:
        void UpdateAvailable(const ui::backend::qt::update::ReleaseInfo& release);
        void UpToDate(const ui::backend::qt::update::ReleaseInfo& latest);
        void CheckFailed(const QString& reason);

    private:
        void OnFinished(QNetworkReply* reply);

        ReleaseVersion current;
        QUrl feedUrl;
        QByteArray userAgent;
        QNetworkAccessManager network;
        QPointer<QNetworkReply> pending;
    };
}
