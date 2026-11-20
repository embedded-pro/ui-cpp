#pragma once

#include "ui/backend/qt/update/UpdateSession.hpp"
#include "ui/backend/qt/update/widgets/UpdateProduct.hpp"
#include <QFrame>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QUrl>

namespace ui::backend::qt::update
{
    class UpdateBanner
        : public QFrame
    {
        Q_OBJECT

    public:
        explicit UpdateBanner(const UpdateProduct& product, QWidget* parent = nullptr);

        void Render(const UpdateState& state);

        QString Message() const;
        QString PrimaryText() const;
        bool PrimaryEnabled() const;

    signals:
        void InstallRequested();
        void CancelRequested();
        void SkipRequested();
        void DismissRequested();

    private:
        void RenderOffer(const UpdateState& state, const QString& version);
        void RenderDownload(const UpdateState& state, const QString& version);
        void RenderReady(const UpdateState& state, const QString& version);
        void RenderFailure(const UpdateState& state, const QString& version);
        void EnablePrimary(bool enabled);
        void ShowButtons(bool primary, bool notes, bool skip, bool cancel, bool dismiss);
        void OnPrimary();

        UpdateProduct product;
        QLabel* message;
        QProgressBar* progress;
        QPushButton* primaryButton;
        QPushButton* notesButton;
        QPushButton* skipButton;
        QPushButton* cancelButton;
        QPushButton* dismissButton;
        QUrl notesUrl;
        QUrl downloadUrl;
        bool primaryInstalls{ false };
    };
}
