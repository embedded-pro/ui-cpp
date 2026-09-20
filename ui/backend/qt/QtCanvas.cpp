#include "ui/backend/qt/QtCanvas.hpp"
#include "ui/backend/qt/QtConversions.hpp"
#include <QFontMetricsF>
#include <QPainter>
#include <algorithm>

namespace ui::backend::qt
{
    QtCanvas::QtCanvas(QPainter& painter)
        : painter(&painter)
    {}

    void QtCanvas::Bind(QPainter& boundPainter)
    {
        painter = &boundPainter;
    }

    void QtCanvas::Save()
    {
        painter->save();
    }

    void QtCanvas::Restore()
    {
        painter->restore();
    }

    void QtCanvas::SetPen(const Pen& pen)
    {
        painter->setPen(ToQt(pen));
    }

    void QtCanvas::SetBrush(const Brush& brush)
    {
        painter->setBrush(ToQt(brush));
    }

    void QtCanvas::SetFont(const FontSpec& font)
    {
        painter->setFont(ToQt(font));
    }

    void QtCanvas::SetAntialiasing(bool enabled)
    {
        painter->setRenderHint(QPainter::Antialiasing, enabled);
        painter->setRenderHint(QPainter::TextAntialiasing, enabled);
    }

    void QtCanvas::SetClip(const Rect& rect)
    {
        painter->setClipRect(ToQt(rect));
    }

    void QtCanvas::ClearClip()
    {
        painter->setClipping(false);
    }

    void QtCanvas::Translate(Point offset)
    {
        painter->translate(ToQt(offset));
    }

    void QtCanvas::Rotate(float degrees)
    {
        painter->rotate(degrees);
    }

    void QtCanvas::DrawLine(Point from, Point to)
    {
        painter->drawLine(ToQt(from), ToQt(to));
    }

    void QtCanvas::DrawPolyline(std::span<const Point> points)
    {
        CopyInto(points);
        painter->drawPolyline(scratch);
    }

    void QtCanvas::DrawPolygon(std::span<const Point> points)
    {
        CopyInto(points);
        painter->drawPolygon(scratch);
    }

    void QtCanvas::DrawRect(const Rect& rect)
    {
        painter->drawRect(ToQt(rect));
    }

    void QtCanvas::FillRect(const Rect& rect, Color color)
    {
        painter->fillRect(ToQt(rect), ToQt(color));
    }

    void QtCanvas::DrawRoundedRect(const Rect& rect, float radiusX, float radiusY)
    {
        painter->drawRoundedRect(ToQt(rect), radiusX, radiusY);
    }

    void QtCanvas::DrawEllipse(Point center, float radiusX, float radiusY)
    {
        painter->drawEllipse(ToQt(center), radiusX, radiusY);
    }

    void QtCanvas::DrawText(Point baseline, std::string_view text)
    {
        painter->drawText(ToQt(baseline), ToQt(text));
    }

    void QtCanvas::DrawText(const Rect& rect, TextAlign align, TextVerticalAlign verticalAlign, std::string_view text)
    {
        painter->drawText(ToQt(rect), static_cast<int>(ToQt(align, verticalAlign)), ToQt(text));
    }

    Size QtCanvas::MeasureText(std::string_view text) const
    {
        const QFontMetricsF metrics{ painter->font() };

        return Size{ static_cast<float>(metrics.horizontalAdvance(ToQt(text))), static_cast<float>(metrics.height()) };
    }

    void QtCanvas::CopyInto(std::span<const Point> points)
    {
        scratch.resize(static_cast<qsizetype>(points.size()));
        std::transform(points.begin(), points.end(), scratch.begin(),
            [](Point point)
            {
                return ToQt(point);
            });
    }
}
