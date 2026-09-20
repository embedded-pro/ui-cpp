#include "ui/theme/Theme.hpp"

namespace ui::theme
{
    namespace
    {
        constexpr std::size_t seriesCount{ 8 };
        constexpr auto firstSeries = static_cast<std::size_t>(ColorRole::Series0);

        const Theme* current{ nullptr };

        // The de-facto palette already in use across the toolbox simulators.
        constexpr std::array<Color, static_cast<std::size_t>(ColorRole::Count)> lightColors{
            Color::Rgb(0xFFFFFF), // Background
            Color::Rgb(0xF5F5F5), // Surface
            Color::Rgb(0x0A0A0A), // ScopeBackground
            Color::Rgb(0xC8C8C8), // GridMajor
            Color::Rgb(0xE6E6E6), // GridMinor
            Color::Rgb(0x646464), // Axis
            Color::Rgb(0x787878), // Crosshair
            Color::Rgb(0x000000), // Text
            Color::Rgb(0x787878), // TextMuted
            Color::Rgb(0xFFFFFF), // TextInverse
            Color::Rgb(0x2980B9), // Series0 blue
            Color::Rgb(0xE74C3C), // Series1 red
            Color::Rgb(0x27AE60), // Series2 green
            Color::Rgb(0x8E44AD), // Series3 purple
            Color::Rgb(0xF39C12), // Series4 orange
            Color::Rgb(0x16A085), // Series5 teal
            Color::Rgb(0xC0392B), // Series6 dark red
            Color::Rgb(0x7F8C8D), // Series7 grey
            Color::Rgb(0x27AE60), // Run
            Color::Rgb(0xC0392B), // Stop
            Color::Rgb(0xFF0000), // EmergencyStop
            Color::Rgb(0xC0392B), // Fault
            Color::Rgb(0xF39C12), // Warning
            Color::Rgb(0x27AE60), // Ok
            Color::Rgb(0x7F8C8D), // Neutral
            Color::Rgb(0x2980B9)  // Accent
        };

        // e-foc's oscilloscope and SVPWM hexagon: dark instrument chrome, higher-contrast traces.
        constexpr std::array<Color, static_cast<std::size_t>(ColorRole::Count)> instrumentColors{
            Color::Rgb(0x1E1E1E), // Background
            Color::Rgb(0x2A2A2A), // Surface
            Color::Rgb(0x0A0A0A), // ScopeBackground
            Color::Rgb(0x3C3C3C), // GridMajor
            Color::Rgb(0x282828), // GridMinor
            Color::Rgb(0x969696), // Axis
            Color::Rgb(0xFFA500), // Crosshair
            Color::Rgb(0xCCCCCC), // Text
            Color::Rgb(0x8C8C8C), // TextMuted
            Color::Rgb(0x000000), // TextInverse
            Color::Rgb(0x0096FF), // Series0
            Color::Rgb(0xFFA500), // Series1
            Color::Rgb(0x00C850), // Series2
            Color::Rgb(0xDC3232), // Series3
            Color::Rgb(0x00DCDC), // Series4
            Color::Rgb(0xDCC800), // Series5
            Color::Rgb(0xC050C0), // Series6
            Color::Rgb(0x969696), // Series7
            Color::Rgb(0x27AE60), // Run
            Color::Rgb(0xC0392B), // Stop
            Color::Rgb(0xFF0000), // EmergencyStop
            Color::Rgb(0xC0392B), // Fault
            Color::Rgb(0xF39C12), // Warning
            Color::Rgb(0x27AE60), // Ok
            Color::Rgb(0x7F8C8D), // Neutral
            Color::Rgb(0x2980B9)  // Accent
        };

        constexpr std::array<FontSpec, static_cast<std::size_t>(FontRole::Count)> defaultFonts{
            FontSpec{ FontFamily::UiDefault, 9, false, false }, // Body
            FontSpec{ FontFamily::UiDefault, 8, false, false }, // Small
            FontSpec{ FontFamily::UiDefault, 10, true, false }, // ChartTitle
            FontSpec{ FontFamily::UiDefault, 8, false, false }, // AxisLabel
            FontSpec{ FontFamily::UiDefault, 8, false, false }, // Legend
            FontSpec{ FontFamily::Monospace, 9, false, false }, // Monospace
            FontSpec{ FontFamily::UiDefault, 9, true, false }   // StatusBold
        };

        constexpr ChartMetrics defaultChartMetrics{};

        const Theme lightTheme{ lightColors, defaultFonts, defaultChartMetrics };
        const Theme instrumentTheme{ instrumentColors, defaultFonts, defaultChartMetrics };
    }

    Color Theme::Series(std::size_t index) const
    {
        return colors[firstSeries + index % seriesCount];
    }

    const Theme& Light()
    {
        return lightTheme;
    }

    const Theme& Instrument()
    {
        return instrumentTheme;
    }

    const Theme& Current()
    {
        return current != nullptr ? *current : lightTheme;
    }

    void SetCurrent(const Theme& theme)
    {
        current = &theme;
    }
}
