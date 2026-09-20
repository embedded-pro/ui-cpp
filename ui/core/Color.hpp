#pragma once

#include <cstdint>

namespace ui
{
    struct Color
    {
        std::uint8_t red{ 0 };
        std::uint8_t green{ 0 };
        std::uint8_t blue{ 0 };
        std::uint8_t alpha{ 255 };

        [[nodiscard]] static constexpr Color Rgb(std::uint32_t value)
        {
            return Color{
                static_cast<std::uint8_t>((value >> 16) & 0xFFu),
                static_cast<std::uint8_t>((value >> 8) & 0xFFu),
                static_cast<std::uint8_t>(value & 0xFFu),
                255
            };
        }

        [[nodiscard]] static constexpr Color Argb(std::uint32_t value)
        {
            return Color{
                static_cast<std::uint8_t>((value >> 16) & 0xFFu),
                static_cast<std::uint8_t>((value >> 8) & 0xFFu),
                static_cast<std::uint8_t>(value & 0xFFu),
                static_cast<std::uint8_t>((value >> 24) & 0xFFu)
            };
        }

        [[nodiscard]] constexpr Color WithAlpha(std::uint8_t value) const
        {
            return Color{ red, green, blue, value };
        }

        [[nodiscard]] constexpr bool operator==(const Color& other) const = default;
    };

    namespace colors
    {
        inline constexpr Color transparent{ 0, 0, 0, 0 };
        inline constexpr Color black{ 0, 0, 0, 255 };
        inline constexpr Color white{ 255, 255, 255, 255 };
    }
}
