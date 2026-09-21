#pragma once

#include "ui/core/Color.hpp"
#include <cstdint>

namespace ui
{
    enum class LineStyle : std::uint8_t
    {
        Solid,
        Dash,
        Dot,
        None
    };

    // Square is the default because it is what QPen already does, so adding this changes no
    // existing widget's output.
    enum class LineCap : std::uint8_t
    {
        Square,
        Flat,
        Round
    };

    struct Pen
    {
        Color color{ colors::black };
        float width{ 1.0f };
        LineStyle style{ LineStyle::Solid };
        LineCap cap{ LineCap::Square };

        [[nodiscard]] constexpr bool operator==(const Pen& other) const = default;
    };

    struct Brush
    {
        Color color{ colors::transparent };

        [[nodiscard]] constexpr bool operator==(const Brush& other) const = default;
    };
}
