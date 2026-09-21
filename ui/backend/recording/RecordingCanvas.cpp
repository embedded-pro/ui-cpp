#include "ui/backend/recording/RecordingCanvas.hpp"
#include <algorithm>

namespace ui::backend::recording
{
    namespace
    {
        // Deterministic and backend-independent by construction: a real text engine would make the
        // recorded command stream vary by platform and defeat the purpose.
        constexpr float advancePerCharacter{ 0.6f };
    }

    void RecordingCanvas::Save()
    {
        Append(CommandKind::Save);
    }

    void RecordingCanvas::Restore()
    {
        Append(CommandKind::Restore);
    }

    void RecordingCanvas::SetPen(const Pen& pen)
    {
        this->pen = pen;
        Append(CommandKind::SetPen).pen = pen;
    }

    void RecordingCanvas::SetBrush(const Brush& brush)
    {
        this->brush = brush;
        Append(CommandKind::SetBrush).brush = brush;
    }

    void RecordingCanvas::SetFont(const FontSpec& font)
    {
        this->font = font;
        Append(CommandKind::SetFont).font = font;
    }

    void RecordingCanvas::SetAntialiasing(bool enabled)
    {
        Append(CommandKind::SetAntialiasing).flag = enabled;
    }

    void RecordingCanvas::SetClip(const Rect& rect)
    {
        Append(CommandKind::SetClip).rect = rect;
    }

    void RecordingCanvas::ClearClip()
    {
        Append(CommandKind::ClearClip);
    }

    void RecordingCanvas::Translate(Point offset)
    {
        Append(CommandKind::Translate).from = offset;
    }

    void RecordingCanvas::Rotate(float degrees)
    {
        Append(CommandKind::Rotate).scalar = degrees;
    }

    void RecordingCanvas::DrawLine(Point from, Point to)
    {
        auto& command = Append(CommandKind::DrawLine);
        command.from = from;
        command.to = to;
        command.pen = pen;
    }

    void RecordingCanvas::DrawPolyline(std::span<const Point> points)
    {
        auto& command = Append(CommandKind::DrawPolyline);
        command.points.assign(points.begin(), points.end());
        command.pen = pen;
    }

    void RecordingCanvas::DrawPolygon(std::span<const Point> points)
    {
        auto& command = Append(CommandKind::DrawPolygon);
        command.points.assign(points.begin(), points.end());
        command.pen = pen;
        command.brush = brush;
    }

    void RecordingCanvas::DrawRect(const Rect& rect)
    {
        auto& command = Append(CommandKind::DrawRect);
        command.rect = rect;
        command.pen = pen;
        command.brush = brush;
    }

    void RecordingCanvas::FillRect(const Rect& rect, Color color)
    {
        auto& command = Append(CommandKind::FillRect);
        command.rect = rect;
        command.color = color;
    }

    void RecordingCanvas::DrawRoundedRect(const Rect& rect, float radiusX, float radiusY)
    {
        auto& command = Append(CommandKind::DrawRoundedRect);
        command.rect = rect;
        command.radiusX = radiusX;
        command.radiusY = radiusY;
        command.pen = pen;
        command.brush = brush;
    }

    void RecordingCanvas::DrawEllipse(Point center, float radiusX, float radiusY)
    {
        auto& command = Append(CommandKind::DrawEllipse);
        command.from = center;
        command.radiusX = radiusX;
        command.radiusY = radiusY;
        command.pen = pen;
        command.brush = brush;
    }

    void RecordingCanvas::DrawText(Point baseline, std::string_view text)
    {
        auto& command = Append(CommandKind::DrawText);
        command.from = baseline;
        command.text.assign(text);
        command.font = font;
        command.pen = pen;
    }

    void RecordingCanvas::DrawText(const Rect& rect, TextAlign align, TextVerticalAlign verticalAlign, std::string_view text)
    {
        auto& command = Append(CommandKind::DrawText);
        command.rect = rect;
        command.align = align;
        command.verticalAlign = verticalAlign;
        command.text.assign(text);
        command.font = font;
        command.pen = pen;
    }

    Size RecordingCanvas::MeasureText(std::string_view text) const
    {
        const auto pointSize = static_cast<float>(font.pointSize);
        return Size{ static_cast<float>(text.size()) * pointSize * advancePerCharacter, pointSize };
    }

    float RecordingCanvas::LineHeight() const
    {
        return static_cast<float>(font.pointSize);
    }

    const std::vector<Command>& RecordingCanvas::Commands() const
    {
        return commands;
    }

    std::size_t RecordingCanvas::CountOf(CommandKind kind) const
    {
        return static_cast<std::size_t>(std::count_if(commands.begin(), commands.end(),
            [kind](const Command& command)
            {
                return command.kind == kind;
            }));
    }

    std::vector<std::string> RecordingCanvas::Texts() const
    {
        std::vector<std::string> result;

        for (const auto& command : commands)
            if (command.kind == CommandKind::DrawText)
                result.push_back(command.text);

        return result;
    }

    const Pen& RecordingCanvas::CurrentPen() const
    {
        return pen;
    }

    void RecordingCanvas::Clear()
    {
        commands.clear();
    }

    Command& RecordingCanvas::Append(CommandKind kind)
    {
        auto& command = commands.emplace_back();
        command.kind = kind;
        return command;
    }
}
