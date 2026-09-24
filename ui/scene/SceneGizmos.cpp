#include "ui/scene/SceneGizmos.hpp"
#include "ui/scene/Clip3.hpp"
#include "ui/theme/Theme.hpp"

namespace ui::scene
{
    namespace
    {
        // An integer loop rather than the original's float accumulator. extent and step are both
        // exactly representable, so the coordinates are bit-identical and the count is not a
        // question of rounding.
        [[nodiscard]] int LineCountFor(const GroundGridConfig& config)
        {
            return static_cast<int>(2.0f * config.extent / config.step);
        }

        // Clipped rather than clamped: with a panned camera the eye can hover over the grid, and
        // a clamped line would smear across the whole viewport.
        void DrawClippedLine(Canvas& canvas, const ViewFrame& frame, Vector3 from, Vector3 to)
        {
            auto viewFrom = frame.ToView(from);
            auto viewTo = frame.ToView(to);

            if (ClipSegmentNear(viewFrom, viewTo, frame.NearDistance()))
                canvas.DrawLine(frame.ProjectView(viewFrom), frame.ProjectView(viewTo));
        }
    }

    void DrawGroundGrid(Canvas& canvas, const ViewFrame& frame, const GroundGridConfig& config)
    {
        if (config.step <= 0.0f || config.extent <= 0.0f)
            return;

        const auto lines = LineCountFor(config);

        canvas.SetBrush(Brush{});
        canvas.SetPen(Pen{ theme::Current().Get(theme::ColorRole::GridMajor), config.lineWidth });

        for (auto i = 0; i <= lines; ++i)
        {
            const auto coordinate = -config.extent + config.step * static_cast<float>(i);

            DrawClippedLine(canvas, frame, Vector3{ coordinate, -config.extent, 0.0f }, Vector3{ coordinate, config.extent, 0.0f });
        }

        for (auto i = 0; i <= lines; ++i)
        {
            const auto coordinate = -config.extent + config.step * static_cast<float>(i);

            DrawClippedLine(canvas, frame, Vector3{ -config.extent, coordinate, 0.0f }, Vector3{ config.extent, coordinate, 0.0f });
        }
    }

    void DrawAxisTriad(Canvas& canvas, const ViewFrame& frame, const AxisTriadConfig& config)
    {
        const auto& theme = theme::Current();
        const auto origin = frame.Project(Vector3{});

        const std::array<Vector3, 3> directions{
            Vector3{ 1.0f, 0.0f, 0.0f },
            Vector3{ 0.0f, 1.0f, 0.0f },
            Vector3{ 0.0f, 0.0f, 1.0f }
        };

        const std::array<theme::ColorRole, 3> roles{
            theme::ColorRole::SceneAxisX,
            theme::ColorRole::SceneAxisY,
            theme::ColorRole::SceneAxisZ
        };

        static constexpr std::array<std::string_view, 3> labels{ "X", "Y", "Z" };

        canvas.SetBrush(Brush{});

        for (std::size_t i = 0; i < directions.size(); ++i)
        {
            canvas.SetPen(Pen{ theme.Get(roles[i]), config.lineWidth });
            canvas.DrawLine(origin, frame.Project(directions[i] * config.length));
        }

        canvas.SetFont(theme.Get(theme::FontRole::Monospace));

        for (std::size_t i = 0; i < directions.size(); ++i)
        {
            canvas.SetPen(Pen{ theme.Get(roles[i]) });
            canvas.DrawText(frame.Project(directions[i] * (config.length + config.labelOffset)), labels[i]);
        }
    }
}
