#pragma once

#include "ui/charts/AxisTransform.hpp"
#include "ui/charts/ChartInteraction.hpp"
#include "ui/charts/Series.hpp"
#include "ui/core/PaintedView.hpp"
#include "ui/theme/Theme.hpp"
#include <array>
#include <cstdint>
#include <span>
#include <string_view>
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
        [[nodiscard]] static Rect PlotAreaFor(const Rect& bounds);

        void DrawPanel(Canvas& canvas, const PanelLayout& layout);
        static void DrawAxes(Canvas& canvas, const Rect& plotArea);
        void DrawGridLines(Canvas& canvas, const Rect& plotArea) const;
        void DrawYLabels(Canvas& canvas, const Rect& plotArea, const PanelBounds& bounds) const;
        void DrawSeries(Canvas& canvas, const Rect& plotArea, const Series& series, const PanelBounds& bounds);
        static void DrawLegend(Canvas& canvas, const Rect& plotArea, const ChartPanel& panel);
        void DrawAxisTitleAndTicks(Canvas& canvas, const Rect& bounds) const;
        void DrawCrosshair(Canvas& canvas) const;
        void DrawCursorReadout(Canvas& canvas, const PanelLayout& layout, float viewPosition) const;

        [[nodiscard]] std::size_t BuildCursorReadout(const ChartPanel& panel, float viewPosition) const;
        [[nodiscard]] std::size_t NearestSampleIndex(float viewPosition, std::size_t count) const;

        const AxisTransform* axis;
        ChartConfig config;

        std::vector<float> axisValues;
        std::vector<ChartPanel> chartPanels;
        ChartInteraction interaction;

        std::vector<PanelLayout> layouts;
        std::vector<Point> polylineScratch;

        // The cursor readout is built during Paint, which must not allocate, so its lines live in
        // fixed storage: one for the axis value plus one per series the theme can colour.
        struct ReadoutLine
        {
            std::array<char, 48> text{};
            std::uint8_t length{ 0 };

            [[nodiscard]] std::string_view Label() const
            {
                return std::string_view{ text.data(), length };
            }
        };

        static constexpr std::size_t maximumTicks{ 32 };
        static constexpr std::size_t maximumGridLines{ 128 };
        static constexpr std::size_t maximumReadoutLines{ 9 };
        mutable std::array<Tick, maximumTicks> ticks{};
        mutable std::array<GridLine, maximumGridLines> gridLines{};
        mutable std::array<ReadoutLine, maximumReadoutLines> readoutLines{};
    };
}
