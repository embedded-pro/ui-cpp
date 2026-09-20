#pragma once

#include "ui/charts/AxisTransform.hpp"
#include "ui/charts/ChartInteraction.hpp"
#include "ui/charts/Series.hpp"
#include "ui/core/PaintedView.hpp"
#include "ui/theme/Theme.hpp"
#include <array>
#include <span>
#include <vector>

namespace ui::charts
{
    struct ChartConfig
    {
        int yLabelDecimals{ 2 };
        int cursorValueDecimals{ 3 };
    };

    // One engine for both the time-domain and frequency-domain charts: everything they differed in
    // lives behind AxisTransform.
    class ChartCore
        : public PaintedView
    {
    public:
        ChartCore(const AxisTransform& axis, ChartConfig config);

        void SetAxisValues(std::span<const float> values);
        void SetPanels(std::vector<ChartPanel> panels);
        void Clear();

        void Paint(Canvas& canvas, const Rect& bounds) override;

        void OnWheel(const WheelEvent& event) override;
        void OnMousePress(const MouseEvent& event) override;
        void OnMouseMove(const MouseEvent& event) override;
        void OnMouseRelease(const MouseEvent& event) override;
        void OnMouseDoubleClick(const MouseEvent& event) override;
        void OnMouseLeave() override;

        [[nodiscard]] const ChartInteraction& Interaction() const;

    private:
        struct PanelLayout
        {
            Rect plotArea;
            PanelBounds bounds;
            std::size_t panelIndex{ 0 };
        };

        void ComputeLayouts(const Rect& bounds);
        [[nodiscard]] PanelBounds ComputeBounds(const ChartPanel& panel) const;
        [[nodiscard]] float ValueToX(float value, const Rect& plotArea) const;
        [[nodiscard]] Rect PlotAreaFor(const Rect& bounds) const;

        void DrawPanel(Canvas& canvas, const PanelLayout& layout);
        void DrawAxes(Canvas& canvas, const Rect& plotArea) const;
        void DrawGridLines(Canvas& canvas, const Rect& plotArea) const;
        void DrawYLabels(Canvas& canvas, const Rect& plotArea, const PanelBounds& bounds) const;
        void DrawSeries(Canvas& canvas, const Rect& plotArea, const Series& series, const PanelBounds& bounds);
        void DrawLegend(Canvas& canvas, const Rect& plotArea, const ChartPanel& panel) const;
        void DrawAxisTitleAndTicks(Canvas& canvas, const Rect& bounds) const;
        void DrawCrosshair(Canvas& canvas) const;

        [[nodiscard]] std::size_t NearestSampleIndex(float viewPosition, std::size_t count) const;

        const AxisTransform* axis;
        ChartConfig config;

        std::vector<float> axisValues;
        std::vector<ChartPanel> chartPanels;
        ChartInteraction interaction;

        std::vector<PanelLayout> layouts;
        std::vector<Point> polylineScratch;

        static constexpr std::size_t maximumTicks{ 32 };
        static constexpr std::size_t maximumGridLines{ 128 };
        mutable std::array<Tick, maximumTicks> ticks{};
        mutable std::array<GridLine, maximumGridLines> gridLines{};
    };
}
