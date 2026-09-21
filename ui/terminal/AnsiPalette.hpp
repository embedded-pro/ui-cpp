#pragma once

#include "ui/core/Color.hpp"
#include "ui/terminal/TerminalTypes.hpp"
#include <array>
#include <cstddef>

namespace ui::terminal
{
    struct AnsiPalette
    {
        std::array<::ui::Color, 8> standard{
            ::ui::Color{ 0x2D, 0x2D, 0x2D },
            ::ui::Color{ 0xE7, 0x4C, 0x3C },
            ::ui::Color{ 0x2E, 0xCC, 0x71 },
            ::ui::Color{ 0xF1, 0xC4, 0x0F },
            ::ui::Color{ 0x34, 0x98, 0xDB },
            ::ui::Color{ 0x9B, 0x59, 0xB6 },
            ::ui::Color{ 0x1A, 0xBC, 0x9C },
            ::ui::Color{ 0xCC, 0xCC, 0xCC }
        };

        std::array<::ui::Color, 8> bright{
            ::ui::Color{ 0x80, 0x80, 0x80 },
            ::ui::Color{ 0xFF, 0x79, 0x6B },
            ::ui::Color{ 0x5F, 0xF9, 0x67 },
            ::ui::Color{ 0xFF, 0xFF, 0x2B },
            ::ui::Color{ 0x6B, 0xC5, 0xFF },
            ::ui::Color{ 0xFF, 0x92, 0xFF },
            ::ui::Color{ 0x5F, 0xFF, 0xEA },
            ::ui::Color{ 0xFF, 0xFF, 0xFF }
        };

        ::ui::Color defaultForeground{ 0xCC, 0xCC, 0xCC };
        ::ui::Color defaultBackground{ 0x1E, 0x1E, 0x1E };

        [[nodiscard]] constexpr ::ui::Color Foreground(Color colour) const
        {
            return Resolve(colour, defaultForeground);
        }

        [[nodiscard]] constexpr ::ui::Color Background(Color colour) const
        {
            return Resolve(colour, defaultBackground);
        }

    private:
        [[nodiscard]] constexpr ::ui::Color Resolve(Color colour, ::ui::Color fallback) const
        {
            const auto index = static_cast<std::size_t>(colour);

            if (index == 0)
                return fallback;

            if (index <= standard.size())
                return standard[index - 1];

            return bright[index - standard.size() - 1];
        }
    };
}
