#pragma once

#include <QByteArray>
#include <QString>
#include <QUrl>

namespace ui::backend::qt::update
{
    struct UpdateProduct
    {
        QString displayName;
        QString shortName;
        QString version;
        QUrl feedUrl;
        QUrl releasesUrl;
        QUrl newIssueUrl;
        QByteArray userAgent;
        QString blockedAction;
    };
}
