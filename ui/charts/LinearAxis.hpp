#pragma once

#include "ui/charts/AxisTransform.hpp"

namespace ui::charts
{
    class LinearAxis
        : public AxisTransform
    {
    public:
        LinearAxis(std::string_view title, int tickCount, int labelDecimals, std::string_view cursorPrefix, std::string_view cursorUnit);

        [[nodiscard]] float ToView(float value) const override;
        [[nodiscard]] float FromView(float view) const override;
        [[nodiscard]] bool IsPlottable(float value) const override;

        [[nodiscard]] AxisRange RangeFor(std::span<const float> values) const override;

        [[nodiscard]] std::size_t Ticks(AxisRange view, std::span<Tick> out) const override;
        [[nodiscard]] std::size_t GridLines(AxisRange view, std::span<GridLine> out) const override;

        [[nodiscard]] std::string_view Title() const override;
        [[nodiscard]] std::size_t FormatCursorValue(float value, std::span<char> out) const override;

        [[nodiscard]] static LinearAxis Time();

    private:
        std::string_view title;
        int tickCount;
        int labelDecimals;
        std::string_view cursorPrefix;
        std::string_view cursorUnit;
    };
}
