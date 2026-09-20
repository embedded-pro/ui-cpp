#include "ui/backend/qt/QtPaintedWidget.hpp"
#include "ui/backend/qt/QtConversions.hpp"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

namespace ui::backend::qt
{
    QtPaintedWidget::QtPaintedWidget(PaintedView& view, QWidget* parent)
        : QWidget(parent)
        , view(view)
    {
        // The hover crosshair needs move events with no button held.
        setMouseTracking(true);
        setFocusPolicy(::Qt::ClickFocus);

        view.onRepaintRequested = [this]
        {
            update();
        };
    }

    QtPaintedWidget::~QtPaintedWidget()
    {
        // The view may outlive the widget; leaving the callback in place would repaint a
        // destroyed QWidget.
        view.onRepaintRequested.Reset();
    }

    void QtPaintedWidget::SetBackgroundRole(theme::ColorRole role)
    {
        backgroundRole = role;
        update();
    }

    void QtPaintedWidget::SetPanCursorEnabled(bool enabled)
    {
        panCursorEnabled = enabled;
    }

    QSize QtPaintedWidget::minimumSizeHint() const
    {
        return ToQtSize(view.MinimumSize());
    }

    void QtPaintedWidget::paintEvent(QPaintEvent* event)
    {
        static_cast<void>(event);

        QPainter painter{ this };
        painter.fillRect(rect(), ToQt(theme::Current().Get(backgroundRole)));

        canvas.Bind(painter);
        view.Paint(canvas, ToUi(QRectF{ rect() }));
    }

    void QtPaintedWidget::mousePressEvent(QMouseEvent* event)
    {
        view.OnMousePress(MouseEvent{ ToUi(event->position()), ToUi(event->button()), ToUi(event->modifiers()) });

        if (panCursorEnabled && event->button() == ::Qt::LeftButton)
            setCursor(::Qt::ClosedHandCursor);
    }

    void QtPaintedWidget::mouseMoveEvent(QMouseEvent* event)
    {
        view.OnMouseMove(MouseEvent{ ToUi(event->position()), ToUi(event->button()), ToUi(event->modifiers()) });
    }

    void QtPaintedWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        view.OnMouseRelease(MouseEvent{ ToUi(event->position()), ToUi(event->button()), ToUi(event->modifiers()) });

        if (panCursorEnabled && event->button() == ::Qt::LeftButton)
            unsetCursor();
    }

    void QtPaintedWidget::mouseDoubleClickEvent(QMouseEvent* event)
    {
        view.OnMouseDoubleClick(MouseEvent{ ToUi(event->position()), ToUi(event->button()), ToUi(event->modifiers()) });
    }

    void QtPaintedWidget::leaveEvent(QEvent* event)
    {
        static_cast<void>(event);
        view.OnMouseLeave();
    }

    void QtPaintedWidget::wheelEvent(QWheelEvent* event)
    {
        // angleDelta().y() passes through unscaled and unflipped: ChartInteraction::Zoom reads a
        // positive delta as zoom-in, which is what the Qt originals relied on.
        view.OnWheel(WheelEvent{ ToUi(event->position()), static_cast<float>(event->angleDelta().y()), ToUi(event->modifiers()) });
    }

    void QtPaintedWidget::keyPressEvent(QKeyEvent* event)
    {
        const auto text = event->text();
        const auto codepoint = text.isEmpty() ? char32_t{ 0 } : static_cast<char32_t>(text.front().unicode());

        view.OnKeyPress(KeyEvent{ ToUiKey(event->key()), codepoint, ToUi(event->modifiers()) });
    }
}
