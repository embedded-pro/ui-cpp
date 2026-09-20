#pragma once

#include "ui/core/Color.hpp"
#include "ui/core/Font.hpp"
#include "ui/core/Geometry.hpp"
#include "ui/core/Pen.hpp"
#include <span>
#include <string_view>

namespace ui
{
    class Canvas
    {
    public:
        Canvas() = default;
        Canvas(const Canvas& other) = delete;
        Canvas& operator=(const Canvas& other) = delete;
        virtual ~Canvas() = default;

        virtual void Save() = 0;
        virtual void Restore() = 0;

        virtual void SetPen(const Pen& pen) = 0;
        virtual void SetBrush(const Brush& brush) = 0;
        virtual void SetFont(const FontSpec& font) = 0;
        virtual void SetAntialiasing(bool enabled) = 0;

        virtual void SetClip(const Rect& rect) = 0;
        virtual void ClearClip() = 0;
        virtual void Translate(Point offset) = 0;
        virtual void Rotate(float degrees) = 0;

        virtual void DrawLine(Point from, Point to) = 0;

        // Batched deliberately. The oscilloscope emits one segment per sample (up to 40 000 per
        // repaint across four channels); a per-segment virtual call would sit on that hot path.
        virtual void DrawPolyline(std::span<const Point> points) = 0;
        virtual void DrawPolygon(std::span<const Point> points) = 0;

        virtual void DrawRect(const Rect& rect) = 0;
        virtual void FillRect(const Rect& rect, Color color) = 0;
        virtual void DrawRoundedRect(const Rect& rect, float radiusX, float radiusY) = 0;
        virtual void DrawEllipse(Point center, float radiusX, float radiusY) = 0;

        virtual void DrawText(Point baseline, std::string_view text) = 0;
        virtual void DrawText(const Rect& rect, TextAlign align, TextVerticalAlign verticalAlign, std::string_view text) = 0;

        [[nodiscard]] virtual Size MeasureText(std::string_view text) const = 0;
    };

    class CanvasStateGuard
    {
    public:
        explicit CanvasStateGuard(Canvas& canvas)
            : canvas(canvas)
        {
            canvas.Save();
        }

        CanvasStateGuard(const CanvasStateGuard& other) = delete;
        CanvasStateGuard& operator=(const CanvasStateGuard& other) = delete;

        ~CanvasStateGuard()
        {
            canvas.Restore();
        }

    private:
        Canvas& canvas;
    };
}
