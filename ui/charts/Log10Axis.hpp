#pragma once

#include "ui/charts/AxisTransform.hpp"

namespace ui::charts
{
    class Log10Axis
        : public AxisTransform
    {
    public:
        [[nodiscard]] float ToView(float value) const override;
        [[nodiscard]] float FromView(float view) const override;
        [[nodiscard]] bool IsPlottable(float value) const override;

        [[nodiscard]] AxisRange RangeFor(std::span<const float> values) const override;

        [[nodiscard]] std::size_t Ticks(AxisRange view, std::span<Tick> out) const override;
        [[nodiscard]] std::size_t GridLines(AxisRange view, std::span<GridLine> out) const override;

        [[nodiscard]] std::string_view Title() const override;
        [[nodiscard]] std::size_t FormatCursorValue(float value, std::span<char> out) const override;

        static std::size_t FormatMagnitude(float value, std::span<char> out);

    private:
        static constexpr float smallestPlottable{ 1e-10f };
        static constexpr int subdivisionsPerDecade{ 9 };
    };
}
