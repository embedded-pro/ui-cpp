#pragma once

#include "ui/core/Color.hpp"
#include "ui/scene/Vector3.hpp"
#include "ui/stage/Ids.hpp"
#include "ui/theme/Theme.hpp"
#include <cstddef>
#include <optional>
#include <vector>

namespace ui::stage
{
    struct TrailStyle
    {
        std::optional<theme::ColorRole> role{ theme::ColorRole::SceneTrail };
        Color color{ colors::black };
        float width{ 1.5f };

        // Skips the depth sort and draws over every solid, like a toolpath preview usually wants.
        bool drawOnTop{ false };
    };

    // A 3D polyline with a fixed capacity: an end-effector trail, a toolpath, a G-code preview.
    // Points are in the frame of `frame`, or in world coordinates when it is invalid, so a path
    // on a moving machine table can travel with the table.
    class Trail
    {
    public:
        explicit Trail(std::size_t capacity, const TrailStyle& style = {}, NodeId frame = {})
            : points(capacity)
            , style(style)
            , frame(frame)
        {}

        // Overwrites the oldest point once full. Never allocates.
        void Push(scene::Vector3 point)
        {
            if (points.empty())
                return;

            points[(first + count) % points.size()] = point;

            if (count < points.size())
                ++count;
            else
                first = (first + 1) % points.size();
        }

        void Clear()
        {
            first = 0;
            count = 0;
        }

        [[nodiscard]] std::size_t Size() const
        {
            return count;
        }

        [[nodiscard]] std::size_t Capacity() const
        {
            return points.size();
        }

        // 0 is the oldest point.
        [[nodiscard]] scene::Vector3 At(std::size_t index) const
        {
            return points[(first + index) % points.size()];
        }

        [[nodiscard]] const TrailStyle& Style() const
        {
            return style;
        }

        void SetStyle(const TrailStyle& newStyle)
        {
            style = newStyle;
        }

        [[nodiscard]] NodeId Frame() const
        {
            return frame;
        }

    private:
        std::vector<scene::Vector3> points;
        std::size_t first{ 0 };
        std::size_t count{ 0 };
        TrailStyle style;
        NodeId frame;
    };
}
