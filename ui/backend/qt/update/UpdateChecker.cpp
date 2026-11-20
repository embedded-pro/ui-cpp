#include "ui/backend/qt/update/UpdateChecker.hpp"
#include <QNetworkReply>
#include <QNetworkRequest>

namespace ui::backend::qt::update
{
    namespace
    {
        constexpr int transferTimeoutMs{ 10000 };
    }

    UpdateChecker::UpdateChecker(const ReleaseVersion& current, const QUrl& feedUrl, const QByteArray& userAgent, QObject* parent)
        : QObject(parent)
        , current(current)
        , feedUrl(feedUrl)
        , userAgent(userAgent)
    {}

    void UpdateChecker::Check()
    {
        if (IsChecking())
            return;

        QNetworkRequest request{ feedUrl };
        request.setRawHeader("Accept", "application/vnd.github+json");
        request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
        request.setTransferTimeout(transferTimeoutMs);

        pending = network.get(request);
        connect(pending, &QNetworkReply::finished, this, [this, reply = pending.data()]()
            {
                OnFinished(reply);
            });
    }

    bool UpdateChecker::IsChecking() const
    {
        return !pending.isNull();
    }

    void UpdateChecker::OnFinished(QNetworkReply* reply)
    {
        reply->deleteLater();
        pending.clear();

        if (reply->error() != QNetworkReply::NoError)
        {
            emit CheckFailed(reply->errorString());
            return;
        }

        const auto evaluation = EvaluateLatestRelease(reply->readAll(), current, CurrentInstallerPlatform());

        switch (evaluation.status)
        {
            case UpdateStatus::updateAvailable:
                emit UpdateAvailable(*evaluation.latest);
                break;
            case UpdateStatus::upToDate:
                emit UpToDate(*evaluation.latest);
                break;
            default:
                emit CheckFailed("the release feed did not contain a valid release");
                break;
        }
    }
}
