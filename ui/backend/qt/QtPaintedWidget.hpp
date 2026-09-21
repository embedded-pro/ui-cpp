#pragma once

#include "ui/backend/qt/QtCanvas.hpp"
#include "ui/core/PaintedView.hpp"
#include "ui/theme/Theme.hpp"
#include <QWidget>

namespace ui::backend::qt
{
    // The single adapter between Qt and every custom-painted view. Written once; the thirteen
    // paintEvent/mouseEvent blocks copy-pasted across the consumer repos collapse into it.
    class QtPaintedWidget
        : public QWidget
        , public PaintedViewHost
    {
        Q_OBJECT

    public:
        explicit QtPaintedWidget(PaintedView& view, QWidget* parent = nullptr);

        // ~PaintedViewHost clears the link, so a view outliving this widget stops repainting a
        // destroyed QWidget without any explicit teardown here.
        ~QtPaintedWidget() override;

        // A view that paints its own opaque background (the oscilloscope) selects
        // ColorRole::ScopeBackground here rather than overpainting the default.
        void SetBackgroundRole(theme::ColorRole role);

        // Off by default: only views that pan on a left drag should show the grab cursor, and the
        // adapter cannot know which those are.
        void SetPanCursorEnabled(bool enabled);

        [[nodiscard]] QSize minimumSizeHint() const override;

        void Invalidate() override;
        void OnViewDestroyed() override;

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;

    private:
        PaintedView* view;
        QtCanvas canvas;
        theme::ColorRole backgroundRole{ theme::ColorRole::Background };
        bool panCursorEnabled{ false };
    };
}
