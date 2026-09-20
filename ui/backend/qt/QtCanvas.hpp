#pragma once

#include "ui/core/Canvas.hpp"
#include <QPolygonF>

class QPainter;

namespace ui::backend::qt
{
    // The Canvas the desktop applications actually render through. It borrows a painter rather
    // than owning one, and is meant to outlive the paint event: QtPaintedWidget keeps one as a
    // member and rebinds it per event so the polyline scratch survives between frames. Every
    // drawing call requires a bound painter.
    class QtCanvas
        : public Canvas
    {
    public:
        QtCanvas() = default;
        explicit QtCanvas(QPainter& painter);

        void Bind(QPainter& painter);

        void Save() override;
        void Restore() override;

        void SetPen(const Pen& pen) override;
        void SetBrush(const Brush& brush) override;
        void SetFont(const FontSpec& font) override;
        void SetAntialiasing(bool enabled) override;

        void SetClip(const Rect& rect) override;
        void ClearClip() override;
        void Translate(Point offset) override;
        void Rotate(float degrees) override;

        void DrawLine(Point from, Point to) override;
        void DrawPolyline(std::span<const Point> points) override;
        void DrawPolygon(std::span<const Point> points) override;

        void DrawRect(const Rect& rect) override;
        void FillRect(const Rect& rect, Color color) override;
        void DrawRoundedRect(const Rect& rect, float radiusX, float radiusY) override;
        void DrawEllipse(Point center, float radiusX, float radiusY) override;

        void DrawText(Point baseline, std::string_view text) override;
        void DrawText(const Rect& rect, TextAlign align, TextVerticalAlign verticalAlign, std::string_view text) override;

        [[nodiscard]] Size MeasureText(std::string_view text) const override;

    private:
        void CopyInto(std::span<const Point> points);

        QPainter* painter{ nullptr };

        // Reused across paints so that a 40 000-point trace does not allocate every frame; the
        // same discipline ChartCore follows for its own scratch buffer.
        QPolygonF scratch;
    };
}
