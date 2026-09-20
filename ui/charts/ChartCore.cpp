#include "ui/charts/ChartCore.hpp"
#include "ui/core/Format.hpp"
#include <algorithm>
#include <limits>

namespace ui::charts
{
    namespace
    {
        constexpr float seriesLineWidth{ 2.0f };
        constexpr float legendWidth{ 160.0f };
        constexpr float legendRowHeight{ 16.0f };
        constexpr float legendSwatchWidth{ 20.0f };
        constexpr float minimumYPadding{ 0.1f };
        constexpr float emptyBound{ 1.0f };
        constexpr float samplesPerPixel{ 2.0f };
        constexpr float axisTitleOffset{ 25.0f };
        constexpr float tickLabelOffset{ 2.0f };
        constexpr float tickLabelHeight{ 14.0f };
        constexpr float tickLabelWidth{ 50.0f };
    }

    ChartCore::ChartCore(const AxisTransform& axis, ChartConfig config)
        : axis(&axis)
        , config(config)
    {}

    void ChartCore::SetAxisValues(std::span<const float> values)
    {
        axisValues.assign(values.begin(), values.end());

        const auto range = axis->RangeFor(axisValues);
        interaction.SetDataRange(range.minimum, range.maximum);

        RequestRepaint();
    }

    void ChartCore::SetPanels(std::vector<ChartPanel> panels)
    {
        chartPanels = std::move(panels);

        auto widest = std::size_t{ 0 };
        for (const auto& panel : chartPanels)
            for (const auto& series : panel.series)
                widest = std::max(widest, series.data.size());

        polylineScratch.reserve(widest + 1);
        layouts.reserve(chartPanels.size());

        RequestRepaint();
    }

    void ChartCore::Clear()
    {
        axisValues.clear();
        chartPanels.clear();
        layouts.clear();
        interaction.SetDataRange(0.0f, 0.0f);

        RequestRepaint();
    }

    const ChartInteraction& ChartCore::Interaction() const
    {
        return interaction;
    }

    Rect ChartCore::PlotAreaFor(const Rect& bounds) const
    {
        const auto& metrics = theme::Current().Charts();

        return Rect{
            bounds.x + static_cast<float>(metrics.leftMargin),
            bounds.y + static_cast<float>(metrics.topMargin),
            bounds.width - static_cast<float>(metrics.leftMargin + metrics.rightMargin),
            bounds.height - static_cast<float>(metrics.topMargin + metrics.bottomMargin)
        };
    }

    void ChartCore::ComputeLayouts(const Rect& bounds)
    {
        layouts.clear();

        const auto& metrics = theme::Current().Charts();
        const auto plotArea = PlotAreaFor(bounds);

        if (plotArea.width <= 0.0f || chartPanels.empty())
            return;

        auto totalWeight = 0;
        for (const auto& panel : chartPanels)
            totalWeight += std::max(panel.heightWeight, 1);

        const auto spacing = static_cast<float>(metrics.panelSpacing);
        const auto availableHeight = plotArea.height - static_cast<float>(chartPanels.size() - 1) * spacing;
        if (availableHeight <= 0.0f)
            return;

        auto currentY = plotArea.y;

        for (std::size_t i = 0; i < chartPanels.size(); ++i)
        {
            const auto weight = static_cast<float>(std::max(chartPanels[i].heightWeight, 1));
            const auto panelHeight = availableHeight * weight / static_cast<float>(totalWeight);

            layouts.push_back(PanelLayout{
                Rect{ plotArea.x, currentY, plotArea.width, panelHeight },
                ComputeBounds(chartPanels[i]),
                i });

            currentY += panelHeight + spacing;
        }
    }

    PanelBounds ChartCore::ComputeBounds(const ChartPanel& panel) const
    {
        PanelBounds bounds{ std::numeric_limits<float>::max(), std::numeric_limits<float>::lowest() };

        auto hasData = false;

        for (const auto& series : panel.series)
        {
            const auto count = std::min(axisValues.size(), series.data.size());

            for (std::size_t i = 0; i < count; ++i)
            {
                if (!axis->IsPlottable(axisValues[i]))
                    continue;

                const auto position = axis->ToView(axisValues[i]);
                if (position < interaction.viewMinimum || position > interaction.viewMaximum)
                    continue;

                bounds.maximumY = std::max(bounds.maximumY, series.data[i]);
                bounds.minimumY = std::min(bounds.minimumY, series.data[i]);
                hasData = true;
            }
        }

        if (!hasData)
            return PanelBounds{ -emptyBound, emptyBound };

        const auto padding = std::max(bounds.Span() * 0.1f, minimumYPadding);
        bounds.maximumY += padding;
        bounds.minimumY -= padding;

        return bounds;
    }

    float ChartCore::ValueToX(float value, const Rect& plotArea) const
    {
        const auto span = interaction.viewMaximum - interaction.viewMinimum;
        if (span <= 0.0f)
            return plotArea.Left();

        const auto ratio = (axis->ToView(value) - interaction.viewMinimum) / span;
        return plotArea.Left() + ratio * plotArea.width;
    }

    void ChartCore::Paint(Canvas& canvas, const Rect& bounds)
    {
        if (chartPanels.empty())
            return;

        ComputeLayouts(bounds);
        if (layouts.empty())
            return;

        canvas.SetAntialiasing(true);

        for (const auto& layout : layouts)
            DrawPanel(canvas, layout);

        DrawAxisTitleAndTicks(canvas, bounds);
        DrawCrosshair(canvas);
    }

    void ChartCore::DrawPanel(Canvas& canvas, const PanelLayout& layout)
    {
        const auto& theme = theme::Current();
        const auto& panel = chartPanels[layout.panelIndex];

        DrawAxes(canvas, layout.plotArea);
        DrawGridLines(canvas, layout.plotArea);

        canvas.SetFont(theme.Get(theme::FontRole::ChartTitle));
        canvas.SetPen(Pen{ theme.Get(theme::ColorRole::Text) });
        canvas.DrawText(Point{ layout.plotArea.Left(), layout.plotArea.Top() - 3.0f }, panel.title);

        DrawYLabels(canvas, layout.plotArea, layout.bounds);

        if (!axisValues.empty())
            for (const auto& series : panel.series)
                DrawSeries(canvas, layout.plotArea, series, layout.bounds);

        DrawLegend(canvas, layout.plotArea, panel);
    }

    void ChartCore::DrawAxes(Canvas& canvas, const Rect& plotArea) const
    {
        canvas.SetPen(Pen{ theme::Current().Get(theme::ColorRole::Axis) });
        canvas.DrawLine(Point{ plotArea.Left(), plotArea.Bottom() }, Point{ plotArea.Right(), plotArea.Bottom() });
        canvas.DrawLine(Point{ plotArea.Left(), plotArea.Bottom() }, Point{ plotArea.Left(), plotArea.Top() });
    }

    void ChartCore::DrawGridLines(Canvas& canvas, const Rect& plotArea) const
    {
        const auto& theme = theme::Current();
        const auto& metrics = theme.Charts();

        const Pen majorPen{ theme.Get(theme::ColorRole::GridMajor), 1.0f, LineStyle::Dash };
        const Pen minorPen{ theme.Get(theme::ColorRole::GridMinor), 1.0f, LineStyle::Dot };

        canvas.SetPen(majorPen);

        for (auto i = 1; i <= metrics.gridLines; ++i)
        {
            const auto y = plotArea.Bottom() - static_cast<float>(i) * plotArea.height / static_cast<float>(metrics.gridLines);
            canvas.DrawLine(Point{ plotArea.Left(), y }, Point{ plotArea.Right(), y });
        }

        const AxisRange view{ interaction.viewMinimum, interaction.viewMaximum };
        const auto count = axis->GridLines(view, gridLines);
        const auto span = view.Span();

        if (span <= 0.0f)
            return;

        for (std::size_t i = 0; i < count; ++i)
        {
            const auto ratio = (gridLines[i].viewPosition - view.minimum) / span;
            const auto x = plotArea.Left() + ratio * plotArea.width;

            if (x <= plotArea.Left() || x >= plotArea.Right())
                continue;

            canvas.SetPen(gridLines[i].major ? majorPen : minorPen);
            canvas.DrawLine(Point{ x, plotArea.Top() }, Point{ x, plotArea.Bottom() });
        }
    }

    void ChartCore::DrawYLabels(Canvas& canvas, const Rect& plotArea, const PanelBounds& bounds) const
    {
        const auto& theme = theme::Current();
        const auto& metrics = theme.Charts();

        canvas.SetFont(theme.Get(theme::FontRole::AxisLabel));
        canvas.SetPen(Pen{ theme.Get(theme::ColorRole::Text) });

        for (auto i = 0; i <= metrics.gridLines; ++i)
        {
            const auto ratio = static_cast<float>(i) / static_cast<float>(metrics.gridLines);
            const auto y = plotArea.Bottom() - ratio * plotArea.height;
            const auto value = bounds.minimumY + ratio * bounds.Span();

            FormatBuffer<32> buffer;
            canvas.DrawText(Point{ plotArea.Left() - 60.0f, y + 4.0f }, buffer.Fixed(value, config.yLabelDecimals));
        }
    }

    void ChartCore::DrawSeries(Canvas& canvas, const Rect& plotArea, const Series& series, const PanelBounds& bounds)
    {
        if (interaction.viewMaximum - interaction.viewMinimum <= 0.0f || series.data.empty())
            return;

        const auto range = bounds.Span() > 0.0f ? bounds.Span() : 1.0f;

        canvas.SetPen(Pen{ series.color, seriesLineWidth });
        canvas.SetClip(plotArea);

        const auto count = std::min(axisValues.size(), series.data.size());
        const auto stride = std::max<std::size_t>(1, count / static_cast<std::size_t>(plotArea.width * samplesPerPixel));

        polylineScratch.clear();

        const auto append = [&](std::size_t i)
        {
            if (!axis->IsPlottable(axisValues[i]))
                return;

            const auto ratio = (series.data[i] - bounds.minimumY) / range;
            polylineScratch.push_back(Point{ ValueToX(axisValues[i], plotArea), plotArea.Bottom() - ratio * plotArea.height });
        };

        for (std::size_t i = 0; i < count; i += stride)
            append(i);

        // The original drew a closing segment to the final sample when the stride skipped it.
        const auto lastIndex = count - 1;
        if (!polylineScratch.empty() && lastIndex % stride != 0)
            append(lastIndex);

        if (polylineScratch.size() > 1)
            canvas.DrawPolyline(polylineScratch);

        canvas.ClearClip();
    }

    void ChartCore::DrawLegend(Canvas& canvas, const Rect& plotArea, const ChartPanel& panel) const
    {
        if (panel.series.size() <= 1)
            return;

        const auto& theme = theme::Current();
        canvas.SetFont(theme.Get(theme::FontRole::Legend));

        const auto legendX = plotArea.Right() - legendWidth;
        const auto legendY = plotArea.Top() + 15.0f;

        for (std::size_t i = 0; i < panel.series.size(); ++i)
        {
            const auto y = legendY + static_cast<float>(i) * legendRowHeight;

            canvas.SetPen(Pen{ panel.series[i].color, seriesLineWidth });
            canvas.DrawLine(Point{ legendX, y }, Point{ legendX + legendSwatchWidth, y });

            canvas.SetPen(Pen{ theme.Get(theme::ColorRole::Text) });
            canvas.DrawText(Point{ legendX + 25.0f, y + 4.0f }, panel.series[i].name);
        }
    }

    void ChartCore::DrawAxisTitleAndTicks(Canvas& canvas, const Rect& bounds) const
    {
        const auto& theme = theme::Current();
        const auto plotArea = PlotAreaFor(bounds);
        const auto lastPanelBottom = layouts.back().plotArea.Bottom();

        canvas.SetFont(theme.Get(theme::FontRole::Body));
        canvas.SetPen(Pen{ theme.Get(theme::ColorRole::Text) });
        canvas.DrawText(Point{ plotArea.Left() + plotArea.width / 2.0f - 20.0f, lastPanelBottom + axisTitleOffset }, axis->Title());

        canvas.SetFont(theme.Get(theme::FontRole::AxisLabel));

        const AxisRange view{ interaction.viewMinimum, interaction.viewMaximum };
        const auto count = axis->Ticks(view, ticks);
        const auto span = view.Span();

        for (std::size_t i = 0; i < count; ++i)
        {
            const auto ratio = span > 0.0f ? (ticks[i].viewPosition - view.minimum) / span : 0.0f;
            const auto x = plotArea.Left() + ratio * plotArea.width;

            const Rect labelRect{ x - tickLabelWidth / 2.0f, lastPanelBottom + tickLabelOffset, tickLabelWidth, tickLabelHeight };
            canvas.DrawText(labelRect, TextAlign::Center, TextVerticalAlign::Middle, ticks[i].Label());
        }
    }

    std::size_t ChartCore::NearestSampleIndex(float viewPosition, std::size_t count) const
    {
        // ToView is monotonic for every axis, so a binary search is correct for both the linear and
        // the logarithmic case; the frequency chart used a linear scan only because it compared in
        // log space.
        const auto begin = axisValues.begin();
        const auto end = begin + static_cast<std::ptrdiff_t>(count);

        const auto it = std::lower_bound(begin, end, viewPosition,
            [this](float sample, float target)
            {
                return axis->ToView(sample) < target;
            });

        if (it == end)
            return count - 1;

        if (it == begin)
            return 0;

        const auto position = static_cast<std::size_t>(std::distance(begin, it));
        const auto previous = position - 1;

        const auto distanceToPrevious = viewPosition - axis->ToView(axisValues[previous]);
        const auto distanceToCurrent = axis->ToView(axisValues[position]) - viewPosition;

        return distanceToPrevious < distanceToCurrent ? previous : position;
    }

    void ChartCore::DrawCrosshair(Canvas& canvas) const
    {
        if (!interaction.showCrosshair || layouts.empty())
            return;

        const auto& theme = theme::Current();
        canvas.SetPen(Pen{ theme.Get(theme::ColorRole::Crosshair), 1.0f, LineStyle::Dash });

        for (const auto& layout : layouts)
            canvas.DrawLine(Point{ interaction.cursorPosition.x, layout.plotArea.Top() },
                Point{ interaction.cursorPosition.x, layout.plotArea.Bottom() });
    }

    void ChartCore::OnWheel(const WheelEvent& event)
    {
        if (layouts.empty())
            return;

        const auto plotArea = layouts.front().plotArea;
        if (plotArea.width <= 0.0f)
            return;

        const auto ratio = Clamp((event.position.x - plotArea.Left()) / plotArea.width, 0.0f, 1.0f);
        interaction.Zoom(event.delta, ratio);

        RequestRepaint();
    }

    void ChartCore::OnMousePress(const MouseEvent& event)
    {
        if (event.button == MouseButton::Left)
            interaction.StartPan(event.position);
    }

    void ChartCore::OnMouseMove(const MouseEvent& event)
    {
        interaction.showCrosshair = true;
        interaction.cursorPosition = event.position;

        if (interaction.IsPanning() && !layouts.empty())
            interaction.UpdatePan(event.position, layouts.front().plotArea.width);

        RequestRepaint();
    }

    void ChartCore::OnMouseRelease(const MouseEvent& event)
    {
        static_cast<void>(event);
        interaction.EndPan();
    }

    void ChartCore::OnMouseDoubleClick(const MouseEvent& event)
    {
        static_cast<void>(event);
        interaction.ResetView();
        RequestRepaint();
    }

    void ChartCore::OnMouseLeave()
    {
        interaction.showCrosshair = false;
        RequestRepaint();
    }
}
