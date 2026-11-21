#pragma once

#include <QApplication>
#include <QDesktopServices>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QTimer>
#include <QUrl>
#include <vector>

namespace ui::backend::qt::update::test
{
    inline void AnswerNextDialog(bool accept)
    {
        auto* timer = new QTimer;
        QObject::connect(timer, &QTimer::timeout, timer, [timer, accept]()
            {
                auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
                if (box == nullptr)
                    return;

                if (accept)
                    box->defaultButton()->click();
                else
                    box->reject();
                timer->deleteLater();
            });
        timer->start(10);
    }

    class UrlCatcher
        : public QObject
    {
        Q_OBJECT

    public:
        UrlCatcher()
        {
            QDesktopServices::setUrlHandler("https", this, "Open");
        }

        ~UrlCatcher() override
        {
            QDesktopServices::unsetUrlHandler("https");
        }

        std::vector<QUrl> urls;

    public slots:

        void Open(const QUrl& url)
        {
            urls.push_back(url);
        }
    };
}
