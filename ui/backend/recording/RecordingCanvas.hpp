#pragma once

#include "ui/core/Canvas.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace ui::backend::recording
{
    enum class CommandKind : std::uint8_t
    {
        Save,
        Restore,
        SetPen,
        SetBrush,
        SetFont,
        SetAntialiasing,
        SetClip,
        ClearClip,
        Translate,
        Rotate,
        DrawLine,
        DrawPolyline,
        DrawPolygon,
        DrawRect,
        FillRect,
        DrawRoundedRect,
        DrawEllipse,
        DrawText
    };

    struct Command
    {
        CommandKind kind{};
        Rect rect{};
        Point from{};
        Point to{};
        float radiusX{ 0.0f };
        float radiusY{ 0.0f };
        float scalar{ 0.0f };
        bool flag{ false };
        Color color{};
        Pen pen{};
        Brush brush{};
        FontSpec font{};
        TextAlign align{ TextAlign::Left };
        TextVerticalAlign verticalAlign{ TextVerticalAlign::Baseline };
        std::string text;
        std::vector<Point> points;
    };

    // The second Canvas implementation. Its existence is what demonstrates the interface is not
    // secretly Qt-shaped, and it is the regression net the consumer repos have never had.
    class RecordingCanvas
        : public Canvas
    {
    public:
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
        [[nodiscard]] float LineHeight() const override;

        [[nodiscard]] const std::vector<Command>& Commands() const;
        [[nodiscard]] std::size_t CountOf(CommandKind kind) const;
        [[nodiscard]] std::vector<std::string> Texts() const;
        [[nodiscard]] const Pen& CurrentPen() const;
        void Clear();

    private:
        Command& Append(CommandKind kind);

        std::vector<Command> commands;
        Pen pen;
        Brush brush;
        FontSpec font;
    };
}
