#pragma once

#include "ui/core/Color.hpp"
#include "ui/core/Font.hpp"
#include <array>
#include <cstdint>

namespace ui::theme
{
    enum class ColorRole : std::uint8_t
    {
        Background,
        Surface,
        ScopeBackground,
        GridMajor,
        GridMinor,
        Axis,
        Crosshair,
        Text,
        TextMuted,
        TextInverse,
        Series0,
        Series1,
        Series2,
        Series3,
        Series4,
        Series5,
        Series6,
        Series7,
        Run,
        Stop,
        EmergencyStop,
        Fault,
        Warning,
        Ok,
        Neutral,
        Accent,
        SceneBackground,
        SceneAxisX,
        SceneAxisY,
        SceneAxisZ,
        Count
    };

    enum class FontRole : std::uint8_t
    {
        Body,
        Small,
        ChartTitle,
        AxisLabel,
        Legend,
        Monospace,
        StatusBold,
        Count
    };

    enum class ButtonRole : std::uint8_t
    {
        Default,
        Primary,
        Start,
        Stop,
        EmergencyStop,
        Reset
    };

    // Hard-coded rather than derived from text metrics: the backends' text engines disagree, and
    // structural layout must not shift between them.
    struct ChartMetrics
    {
        int leftMargin{ 65 };
        int rightMargin{ 20 };
        int topMargin{ 15 };
        int bottomMargin{ 35 };
        int panelSpacing{ 45 };
        int gridLines{ 5 };
    };

    class Theme
    {
    public:
        constexpr Theme(const std::array<Color, static_cast<std::size_t>(ColorRole::Count)>& colors,
            const std::array<FontSpec, static_cast<std::size_t>(FontRole::Count)>& fonts,
            const ChartMetrics& chartMetrics)
            : colors(colors)
            , fonts(fonts)
            , chartMetrics(chartMetrics)
        {}

        [[nodiscard]] constexpr Color Get(ColorRole role) const
        {
            return colors[static_cast<std::size_t>(role)];
        }

        [[nodiscard]] constexpr FontSpec Get(FontRole role) const
        {
            return fonts[static_cast<std::size_t>(role)];
        }

        [[nodiscard]] constexpr const ChartMetrics& Charts() const
        {
            return chartMetrics;
        }

        [[nodiscard]] Color Series(std::size_t index) const;

    private:
        std::array<Color, static_cast<std::size_t>(ColorRole::Count)> colors;
        std::array<FontSpec, static_cast<std::size_t>(FontRole::Count)> fonts;
        ChartMetrics chartMetrics;
    };

    [[nodiscard]] const Theme& Light();
    [[nodiscard]] const Theme& Instrument();

    [[nodiscard]] const Theme& Current();
    void SetCurrent(const Theme& theme);
}
