#pragma once

#include <algorithm>
#include <cmath>

namespace ui
{
    struct Point
    {
        float x{ 0.0f };
        float y{ 0.0f };

        [[nodiscard]] constexpr Point Translated(float dx, float dy) const
        {
            return Point{ x + dx, y + dy };
        }
    };

    struct Size
    {
        float width{ 0.0f };
        float height{ 0.0f };

        [[nodiscard]] constexpr bool IsEmpty() const
        {
            return width <= 0.0f || height <= 0.0f;
        }
    };

    // Bottom() is y + height, unlike QRect::bottom() which is y + height - 1. Ported widgets
    // therefore differ from their Qt originals by one pixel; see doc/canvas.md.
    struct Rect
    {
        float x{ 0.0f };
        float y{ 0.0f };
        float width{ 0.0f };
        float height{ 0.0f };

        [[nodiscard]] constexpr float Left() const
        {
            return x;
        }

        [[nodiscard]] constexpr float Right() const
        {
            return x + width;
        }

        [[nodiscard]] constexpr float Top() const
        {
            return y;
        }

        [[nodiscard]] constexpr float Bottom() const
        {
            return y + height;
        }

        [[nodiscard]] constexpr Point Center() const
        {
            return Point{ x + width * 0.5f, y + height * 0.5f };
        }

        [[nodiscard]] constexpr Size Extent() const
        {
            return Size{ width, height };
        }

        [[nodiscard]] constexpr bool IsEmpty() const
        {
            return width <= 0.0f || height <= 0.0f;
        }

        [[nodiscard]] constexpr bool Contains(Point point) const
        {
            return point.x >= Left() && point.x <= Right() && point.y >= Top() && point.y <= Bottom();
        }

        [[nodiscard]] constexpr Rect Adjusted(float dLeft, float dTop, float dRight, float dBottom) const
        {
            return Rect{ x + dLeft, y + dTop, width - dLeft + dRight, height - dTop + dBottom };
        }

        [[nodiscard]] constexpr Rect Intersected(const Rect& other) const
        {
            const auto left = std::max(Left(), other.Left());
            const auto top = std::max(Top(), other.Top());
            const auto right = std::min(Right(), other.Right());
            const auto bottom = std::min(Bottom(), other.Bottom());

            if (right <= left || bottom <= top)
                return Rect{};

            return Rect{ left, top, right - left, bottom - top };
        }
    };

    struct Transform
    {
        float translateX{ 0.0f };
        float translateY{ 0.0f };
        float rotationDegrees{ 0.0f };
        float scaleX{ 1.0f };
        float scaleY{ 1.0f };

        [[nodiscard]] Point Apply(Point point) const
        {
            const auto radians = rotationDegrees * 3.14159265358979323846f / 180.0f;
            const auto cosine = std::cos(radians);
            const auto sine = std::sin(radians);

            const auto scaledX = point.x * scaleX;
            const auto scaledY = point.y * scaleY;

            return Point{
                scaledX * cosine - scaledY * sine + translateX,
                scaledX * sine + scaledY * cosine + translateY
            };
        }
    };

    [[nodiscard]] constexpr float Lerp(float from, float to, float ratio)
    {
        return from + (to - from) * ratio;
    }

    [[nodiscard]] constexpr float Clamp(float value, float lowest, float highest)
    {
        return value < lowest ? lowest : (value > highest ? highest : value);
    }
}
