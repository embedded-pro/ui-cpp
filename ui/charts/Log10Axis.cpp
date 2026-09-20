#include "ui/charts/Log10Axis.hpp"
#include "ui/core/Format.hpp"
#include <algorithm>
#include <cmath>

namespace ui::charts
{
    float Log10Axis::ToView(float value) const
    {
        return std::log10(std::max(value, smallestPlottable));
    }

    float Log10Axis::FromView(float view) const
    {
        return std::pow(10.0f, view);
    }

    bool Log10Axis::IsPlottable(float value) const
    {
        return value > 0.0f;
    }

    AxisRange Log10Axis::RangeFor(std::span<const float> values) const
    {
        if (values.empty())
            return AxisRange{ 0.0f, 0.0f };

        auto smallest = *std::ranges::min_element(values);
        const auto largest = *std::ranges::max_element(values);

        if (smallest <= 0.0f)
            smallest = 1.0f;

        return AxisRange{ std::log10(smallest), std::log10(largest) };
    }

    std::size_t Log10Axis::FormatMagnitude(float value, std::span<char> out)
    {
        return value >= 1000.0f
                   ? FormatInto(out, "{:.3g}k", value / 1000.0f)
                   : FormatInto(out, "{:.3g}", value);
    }

    // The endpoints are always labelled; whole decades strictly inside the view are labelled too.
    std::size_t Log10Axis::Ticks(AxisRange view, std::span<Tick> out) const
    {
        if (view.Span() <= 0.0f || out.empty())
            return 0;

        std::size_t count = 0;

        const auto emit = [&](float viewPosition)
        {
            if (count >= out.size())
                return;

            std::array<char, 24> scratch{};
            const auto length = FormatMagnitude(FromView(viewPosition), scratch);

            out[count].viewPosition = viewPosition;
            out[count].SetLabel(std::string_view{ scratch.data(), length });
            ++count;
        };

        emit(view.minimum);

        const auto firstDecade = static_cast<int>(std::floor(view.minimum)) + 1;
        const auto lastDecade = static_cast<int>(std::ceil(view.maximum)) - 1;

        for (auto decade = firstDecade; decade <= lastDecade; ++decade)
        {
            const auto position = static_cast<float>(decade);
            if (position > view.minimum && position < view.maximum)
                emit(position);
        }

        emit(view.maximum);

        return count;
    }

    std::size_t Log10Axis::GridLines(AxisRange view, std::span<GridLine> out) const
    {
        if (view.Span() <= 0.0f)
            return 0;

        std::size_t count = 0;

        const auto firstDecade = static_cast<int>(std::floor(view.minimum));
        const auto lastDecade = static_cast<int>(std::ceil(view.maximum));

        for (auto decade = firstDecade; decade <= lastDecade; ++decade)
        {
            for (auto subdivision = 1; subdivision <= subdivisionsPerDecade; ++subdivision)
            {
                if (count == out.size())
                    return count;

                const auto value = static_cast<float>(subdivision) * std::pow(10.0f, static_cast<float>(decade));
                const auto position = ToView(value);

                if (position <= view.minimum || position >= view.maximum)
                    continue;

                out[count].viewPosition = position;
                out[count].major = subdivision == 1;
                ++count;
            }
        }

        return count;
    }

    std::string_view Log10Axis::Title() const
    {
        return "Frequency (Hz)";
    }

    std::size_t Log10Axis::FormatCursorValue(float value, std::span<char> out) const
    {
        return value >= 1000.0f
                   ? FormatInto(out, "f = {:.2f} kHz", value / 1000.0f)
                   : FormatInto(out, "f = {:.1f} Hz", value);
    }
}
