#pragma once

#include "ui/core/Color.hpp"
#include <cstdint>

namespace ui
{
    enum class LineStyle : std::uint8_t
    {
        Solid,
        Dash,
        Dot
    };

    struct Pen
    {
        Color color{ colors::black };
        float width{ 1.0f };
        LineStyle style{ LineStyle::Solid };

        [[nodiscard]] constexpr bool operator==(const Pen& other) const = default;
    };

    struct Brush
    {
        Color color{ colors::transparent };

        [[nodiscard]] constexpr bool operator==(const Brush& other) const = default;
    };
}
