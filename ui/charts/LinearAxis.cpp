#include "ui/charts/LinearAxis.hpp"
#include "ui/core/Format.hpp"
#include <algorithm>
#include <cstdio>

namespace ui::charts
{
    LinearAxis::LinearAxis(std::string_view title, int tickCount, int labelDecimals, std::string_view cursorPrefix, std::string_view cursorUnit)
        : title(title)
        , tickCount(tickCount)
        , labelDecimals(labelDecimals)
        , cursorPrefix(cursorPrefix)
        , cursorUnit(cursorUnit)
    {}

    LinearAxis LinearAxis::Time()
    {
        return LinearAxis{ "Time (s)", 5, 2, "t = ", " s" };
    }

    float LinearAxis::ToView(float value) const
    {
        return value;
    }

    float LinearAxis::FromView(float view) const
    {
        return view;
    }

    bool LinearAxis::IsPlottable(float value) const
    {
        static_cast<void>(value);
        return true;
    }

    // The time-domain chart anchored the range at zero rather than at the first sample.
    AxisRange LinearAxis::RangeFor(std::span<const float> values) const
    {
        if (values.empty())
            return AxisRange{ 0.0f, 0.0f };

        return AxisRange{ 0.0f, values.back() };
    }

    std::size_t LinearAxis::Ticks(AxisRange view, std::span<Tick> out) const
    {
        const auto count = std::min(static_cast<std::size_t>(tickCount) + 1, out.size());

        for (std::size_t i = 0; i < count; ++i)
        {
            const auto ratio = static_cast<float>(i) / static_cast<float>(tickCount);
            const auto value = view.Span() > 0.0f ? view.minimum + ratio * view.Span() : 0.0f;

            out[i].viewPosition = view.minimum + ratio * view.Span();

            FormatBuffer<32> buffer;
            out[i].SetLabel(buffer.Fixed(value, labelDecimals));
        }

        return count;
    }

    std::size_t LinearAxis::GridLines(AxisRange view, std::span<GridLine> out) const
    {
        if (view.Span() <= 0.0f)
            return 0;

        const auto count = std::min(static_cast<std::size_t>(tickCount), out.size());

        for (std::size_t i = 0; i < count; ++i)
        {
            const auto ratio = static_cast<float>(i + 1) / static_cast<float>(tickCount);
            out[i].viewPosition = view.minimum + ratio * view.Span();
            out[i].major = true;
        }

        return count;
    }

    std::string_view LinearAxis::Title() const
    {
        return title;
    }

    std::size_t LinearAxis::FormatCursorValue(float value, std::span<char> out) const
    {
        const auto written = std::snprintf(out.data(), out.size(), "%.*s%.3f%.*s",
            static_cast<int>(cursorPrefix.size()), cursorPrefix.data(),
            static_cast<double>(value),
            static_cast<int>(cursorUnit.size()), cursorUnit.data());

        return written < 0 ? 0 : std::min(static_cast<std::size_t>(written), out.size() - 1);
    }
}
