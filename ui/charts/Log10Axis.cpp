#include "ui/charts/Log10Axis.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

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

        auto smallest = *std::min_element(values.begin(), values.end());
        const auto largest = *std::max_element(values.begin(), values.end());

        if (smallest <= 0.0f)
            smallest = 1.0f;

        return AxisRange{ std::log10(smallest), std::log10(largest) };
    }

    std::size_t Log10Axis::FormatMagnitude(float value, std::span<char> out)
    {
        const auto written = value >= 1000.0f
                                 ? std::snprintf(out.data(), out.size(), "%.3gk", static_cast<double>(value / 1000.0f))
                                 : std::snprintf(out.data(), out.size(), "%.3g", static_cast<double>(value));

        return written < 0 ? 0 : std::min(static_cast<std::size_t>(written), out.size() - 1);
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

        for (auto decade = firstDecade; decade <= lastDecade && count < out.size(); ++decade)
            for (auto subdivision = 1; subdivision <= subdivisionsPerDecade && count < out.size(); ++subdivision)
            {
                const auto value = static_cast<float>(subdivision) * std::pow(10.0f, static_cast<float>(decade));
                const auto position = ToView(value);

                if (position <= view.minimum || position >= view.maximum)
                    continue;

                out[count].viewPosition = position;
                out[count].major = subdivision == 1;
                ++count;
            }

        return count;
    }

    std::string_view Log10Axis::Title() const
    {
        return "Frequency (Hz)";
    }

    std::size_t Log10Axis::FormatCursorValue(float value, std::span<char> out) const
    {
        const auto written = value >= 1000.0f
                                 ? std::snprintf(out.data(), out.size(), "f = %.2f kHz", static_cast<double>(value / 1000.0f))
                                 : std::snprintf(out.data(), out.size(), "f = %.1f Hz", static_cast<double>(value));

        return written < 0 ? 0 : std::min(static_cast<std::size_t>(written), out.size() - 1);
    }
}
