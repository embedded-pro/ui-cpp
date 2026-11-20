#include "ui/backend/qt/update/widgets/UpdateBanner.hpp"
#include "ui/backend/qt/QtTheme.hpp"
#include "ui/theme/Theme.hpp"
#include <QDesktopServices>
#include <QHBoxLayout>

namespace ui::backend::qt::update
{
    namespace
    {
        QString Megabytes(qint64 bytes)
        {
            return QString::number(static_cast<double>(bytes) / (1024.0 * 1024.0), 'f', 1);
        }
    }

    UpdateBanner::UpdateBanner(const UpdateProduct& product, QWidget* parent)
        : QFrame(parent)
        , product(product)
    {
        setFrameShape(QFrame::StyledPanel);
        setAccessibleName("Update");

        auto* layout = new QHBoxLayout(this);
        message = new QLabel();
        message->setWordWrap(true);
        layout->addWidget(message, 1);

        progress = new QProgressBar();
        progress->setAccessibleName("Download progress");
        progress->setMaximumWidth(220);
        layout->addWidget(progress);

        primaryButton = new QPushButton();
        notesButton = new QPushButton("&Release Notes");
        skipButton = new QPushButton("S&kip This Version");
        skipButton->setToolTip("Do not offer this version again at automatic checks");
        cancelButton = new QPushButton("Cancel");
        dismissButton = new QPushButton("Later");
        dismissButton->setToolTip("Hide this message until the next check");
        for (auto* button : { primaryButton, notesButton, skipButton, cancelButton, dismissButton })
            layout->addWidget(button);

        connect(primaryButton, &QPushButton::clicked, this, &UpdateBanner::OnPrimary);
        connect(notesButton, &QPushButton::clicked, this, [this]()
            {
                QDesktopServices::openUrl(notesUrl);
            });
        connect(skipButton, &QPushButton::clicked, this, &UpdateBanner::SkipRequested);
        connect(cancelButton, &QPushButton::clicked, this, &UpdateBanner::CancelRequested);
        connect(dismissButton, &QPushButton::clicked, this, &UpdateBanner::DismissRequested);

        hide();
    }

    void UpdateBanner::Render(const UpdateState& state)
    {
        if (state.phase == UpdatePhase::idle || !state.release)
        {
            hide();
            return;
        }

        notesUrl = state.release->pageUrl;
        downloadUrl = state.release->installer ? state.release->installer->url : state.release->pageUrl;
        const auto version = QString::fromStdString(state.release->version.ToString());
        progress->setVisible(state.phase == UpdatePhase::downloading);
        StyleStatusLabel(*message, state.phase == UpdatePhase::failed ? theme::StatusLevel::Warning : theme::StatusLevel::Ok);

        switch (state.phase)
        {
            case UpdatePhase::available:
                RenderOffer(state, version);
                break;
            case UpdatePhase::downloading:
                RenderDownload(state, version);
                break;
            case UpdatePhase::ready:
                RenderReady(state, version);
                break;
            case UpdatePhase::installing:
                message->setText(QString("Installing %1 %2 — %3 restarts when it is done.").arg(product.displayName, version, product.shortName));
                ShowButtons(false, false, false, false, false);
                break;
            case UpdatePhase::failed:
                RenderFailure(state, version);
                break;
            default:
                break;
        }

        show();
    }

    QString UpdateBanner::Message() const
    {
        return message->text();
    }

    QString UpdateBanner::PrimaryText() const
    {
        return primaryButton->isVisibleTo(this) ? primaryButton->text() : QString{};
    }

    bool UpdateBanner::PrimaryEnabled() const
    {
        return primaryButton->isEnabled();
    }

    void UpdateBanner::RenderOffer(const UpdateState& state, const QString& version)
    {
        message->setText(QString("%1 %2 is available — you have %3.").arg(product.displayName, version, product.version));
        primaryInstalls = state.automaticInstall;
        primaryButton->setText(state.automaticInstall ? "&Install Update…" : (state.release->installer ? "Do&wnload Installer" : "Open Do&wnload Page"));
        EnablePrimary(true);
        primaryButton->setToolTip(state.automaticInstall ? "Download, verify and install this version, then restart" : "This installation cannot update itself; download the installer instead");
        ShowButtons(true, true, true, false, true);
    }

    void UpdateBanner::RenderDownload(const UpdateState& state, const QString& version)
    {
        const auto total = state.total > 0 ? state.total : 0;
        progress->setRange(0, total > 0 ? 1000 : 0);
        progress->setValue(total > 0 ? static_cast<int>(state.received * 1000 / total) : 0);
        message->setText(total > 0 ? QString("Downloading %1 — %2 of %3 MB").arg(version, Megabytes(state.received), Megabytes(total))
                                   : QString("Downloading %1 — %2 MB").arg(version, Megabytes(state.received)));
        ShowButtons(false, true, false, true, false);
    }

    void UpdateBanner::RenderReady(const UpdateState& state, const QString& version)
    {
        primaryInstalls = true;
        primaryButton->setText("&Install and Restart");
        EnablePrimary(!state.blocked);
        primaryButton->setToolTip(state.blocked ? QString("%1 first — installing closes %2").arg(product.blockedAction, product.shortName) : QString{});
        message->setText(state.blocked ? QString("%1 is downloaded and verified. %2 to install it.").arg(version, product.blockedAction)
                                       : QString("%1 is downloaded and verified.").arg(version));
        ShowButtons(true, true, false, true, false);
    }

    void UpdateBanner::RenderFailure(const UpdateState& state, const QString& version)
    {
        message->setText(QString("Could not update to %1: %2.").arg(version, state.failure));
        primaryInstalls = state.automaticInstall;
        primaryButton->setText(state.automaticInstall ? "&Try Again" : "Open Do&wnload Page");
        EnablePrimary(true);
        primaryButton->setToolTip({});
        if (!state.automaticInstall)
            downloadUrl = state.release->pageUrl;
        ShowButtons(true, true, false, false, true);
    }

    void UpdateBanner::EnablePrimary(bool enabled)
    {
        primaryButton->setEnabled(enabled);
        StyleButton(*primaryButton, enabled ? theme::ButtonRole::Primary : theme::ButtonRole::Default);
    }

    void UpdateBanner::ShowButtons(bool primary, bool notes, bool skip, bool cancel, bool dismiss)
    {
        primaryButton->setVisible(primary);
        notesButton->setVisible(notes);
        skipButton->setVisible(skip);
        cancelButton->setVisible(cancel);
        dismissButton->setVisible(dismiss);
    }

    void UpdateBanner::OnPrimary()
    {
        if (primaryInstalls)
            emit InstallRequested();
        else
            QDesktopServices::openUrl(downloadUrl);
    }
}
