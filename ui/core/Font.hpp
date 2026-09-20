#pragma once

#include <cstdint>

namespace ui
{
    // A closed set rather than a family name: the four consumer repos use exactly two fonts, and
    // font-by-name lookup is not portable to non-desktop backends.
    enum class FontFamily : std::uint8_t
    {
        UiDefault,
        Monospace
    };

    struct FontSpec
    {
        FontFamily family{ FontFamily::UiDefault };
        std::uint8_t pointSize{ 9 };
        bool bold{ false };
        bool italic{ false };

        [[nodiscard]] constexpr bool operator==(const FontSpec& other) const = default;
    };

    enum class TextAlign : std::uint8_t
    {
        Left,
        Center,
        Right
    };

    enum class TextVerticalAlign : std::uint8_t
    {
        Top,
        Middle,
        Baseline,
        Bottom
    };
}
