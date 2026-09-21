#include "ui/backend/recording/RecordingCanvas.hpp"
#include "ui/backend/recording/RecordingPaintedHost.hpp"
#include "ui/charts/ChartCore.hpp"
#include "ui/charts/LinearAxis.hpp"
#include "ui/charts/Log10Axis.hpp"
#include <gmock/gmock.h>

namespace
{
    using ui::backend::recording::CommandKind;

    std::vector<ui::charts::ChartPanel> OnePanel(std::size_t seriesCount, std::size_t sampleCount)
    {
        ui::charts::ChartPanel panel;
        panel.title = "Step response";
        panel.yAxisLabel = "Amplitude";

        for (std::size_t s = 0; s < seriesCount; ++s)
        {
            ui::charts::Series series;
            series.name = "series" + std::to_string(s);
            series.color = ui::theme::Light().Series(s);

            for (std::size_t i = 0; i < sampleCount; ++i)
                series.data.push_back(static_cast<float>(i) * 0.1f);

            panel.series.push_back(std::move(series));
        }

        return { std::move(panel) };
    }

    std::vector<float> LinearAxisValues(std::size_t count)
    {
        std::vector<float> values;
        for (std::size_t i = 0; i < count; ++i)
            values.push_back(static_cast<float>(i));
        return values;
    }

    std::size_t CrosshairLineCount(const ui::backend::recording::RecordingCanvas& canvas)
    {
        const auto crosshair = ui::theme::Current().Get(ui::theme::ColorRole::Crosshair);
        std::size_t count{ 0 };

        for (const auto& command : canvas.Commands())
            if (command.kind == CommandKind::DrawLine && command.pen.color == crosshair)
                ++count;

        return count;
    }

    const ui::backend::recording::Command* ReadoutBackground(const ui::backend::recording::RecordingCanvas& canvas)
    {
        for (const auto& command : canvas.Commands())
            if (command.kind == CommandKind::DrawRoundedRect)
                return &command;

        return nullptr;
    }

    class ChartCoreTest
        : public ::testing::Test
    {
    protected:
        void HoverAt(ui::Point position, std::size_t seriesCount = 1)
        {
            chart.SetAxisValues(LinearAxisValues(20));
            chart.SetPanels(OnePanel(seriesCount, 20));
            chart.OnMouseMove(ui::MouseEvent{ position, ui::MouseButton::None, {} });
            chart.Paint(canvas, bounds);
        }

        ui::backend::recording::RecordingCanvas canvas;
        ui::charts::LinearAxis axis{ ui::charts::LinearAxis::Time() };
        ui::charts::ChartCore chart{ axis, ui::charts::ChartConfig{} };

        static constexpr ui::Rect bounds{ 0.0f, 0.0f, 800.0f, 600.0f };

        // PlotAreaFor with the default ChartMetrics: x in [65, 780], y in [15, 565].
        static constexpr ui::Point insidePlot{ 441.0f, 300.0f };
    };

    class FrequencyChartCoreTest
        : public ::testing::Test
    {
    protected:
        ui::backend::recording::RecordingCanvas canvas;
        ui::charts::Log10Axis axis;
        ui::charts::ChartCore chart{ axis, ui::charts::ChartConfig{ 1, 2 } };

        static constexpr ui::Rect bounds{ 0.0f, 0.0f, 800.0f, 600.0f };
    };
}

TEST_F(ChartCoreTest, PaintingWithoutPanelsDrawsNothing)
{
    chart.Paint(canvas, bounds);

    EXPECT_TRUE(canvas.Commands().empty());
}

TEST_F(ChartCoreTest, PaintingWithoutRoomDrawsNothing)
{
    chart.SetAxisValues(LinearAxisValues(10));
    chart.SetPanels(OnePanel(1, 10));
    chart.Paint(canvas, ui::Rect{ 0.0f, 0.0f, 10.0f, 10.0f });

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawPolyline), 0u);
}

TEST_F(ChartCoreTest, EachSeriesBecomesExactlyOnePolyline)
{
    chart.SetAxisValues(LinearAxisValues(50));
    chart.SetPanels(OnePanel(3, 50));
    chart.Paint(canvas, bounds);

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawPolyline), 3u);
}

TEST_F(ChartCoreTest, ThePolylineCarriesEverySample)
{
    chart.SetAxisValues(LinearAxisValues(40));
    chart.SetPanels(OnePanel(1, 40));
    chart.Paint(canvas, bounds);

    for (const auto& command : canvas.Commands())
    {
        if (command.kind == CommandKind::DrawPolyline)
        {
            EXPECT_EQ(command.points.size(), 40u);
        }
    }
}

TEST_F(ChartCoreTest, SeriesAreDrawnInTheirOwnColour)
{
    chart.SetAxisValues(LinearAxisValues(20));
    chart.SetPanels(OnePanel(2, 20));
    chart.Paint(canvas, bounds);

    std::vector<ui::Color> drawn;
    for (const auto& command : canvas.Commands())
        if (command.kind == CommandKind::DrawPolyline)
            drawn.push_back(command.pen.color);

    ASSERT_EQ(drawn.size(), 2u);
    EXPECT_EQ(drawn[0], ui::theme::Light().Series(0));
    EXPECT_EQ(drawn[1], ui::theme::Light().Series(1));
}

TEST_F(ChartCoreTest, SeriesAreClippedToThePlotArea)
{
    chart.SetAxisValues(LinearAxisValues(20));
    chart.SetPanels(OnePanel(1, 20));
    chart.Paint(canvas, bounds);

    EXPECT_EQ(canvas.CountOf(CommandKind::SetClip), 1u);
    EXPECT_EQ(canvas.CountOf(CommandKind::ClearClip), 1u);
}

TEST_F(ChartCoreTest, TheAxisTitleIsDrawn)
{
    chart.SetAxisValues(LinearAxisValues(20));
    chart.SetPanels(OnePanel(1, 20));
    chart.Paint(canvas, bounds);

    EXPECT_THAT(canvas.Texts(), ::testing::Contains("Time (s)"));
}

TEST_F(ChartCoreTest, ThePanelTitleIsDrawn)
{
    chart.SetAxisValues(LinearAxisValues(20));
    chart.SetPanels(OnePanel(1, 20));
    chart.Paint(canvas, bounds);

    EXPECT_THAT(canvas.Texts(), ::testing::Contains("Step response"));
}

TEST_F(ChartCoreTest, TickLabelsUseTheConfiguredDecimals)
{
    chart.SetAxisValues(LinearAxisValues(11));
    chart.SetPanels(OnePanel(1, 11));
    chart.Paint(canvas, bounds);

    EXPECT_THAT(canvas.Texts(), ::testing::Contains("0.00"));
    EXPECT_THAT(canvas.Texts(), ::testing::Contains("10.00"));
}

TEST_F(ChartCoreTest, ASingleSeriesHasNoLegend)
{
    chart.SetAxisValues(LinearAxisValues(20));
    chart.SetPanels(OnePanel(1, 20));
    chart.Paint(canvas, bounds);

    EXPECT_THAT(canvas.Texts(), ::testing::Not(::testing::Contains("series0")));
}

TEST_F(ChartCoreTest, TwoSeriesGetALegend)
{
    chart.SetAxisValues(LinearAxisValues(20));
    chart.SetPanels(OnePanel(2, 20));
    chart.Paint(canvas, bounds);

    EXPECT_THAT(canvas.Texts(), ::testing::Contains("series0"));
    EXPECT_THAT(canvas.Texts(), ::testing::Contains("series1"));
}

TEST_F(ChartCoreTest, PanelsSplitTheHeightByWeight)
{
    auto panels = OnePanel(1, 20);
    panels.push_back(panels.front());
    panels[0].heightWeight = 3;
    panels[1].heightWeight = 1;

    chart.SetAxisValues(LinearAxisValues(20));
    chart.SetPanels(std::move(panels));
    chart.Paint(canvas, bounds);

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawPolyline), 2u);
}

TEST_F(ChartCoreTest, ClearRemovesEverythingDrawn)
{
    chart.SetAxisValues(LinearAxisValues(20));
    chart.SetPanels(OnePanel(1, 20));
    chart.Clear();
    chart.Paint(canvas, bounds);

    EXPECT_TRUE(canvas.Commands().empty());
}

TEST_F(ChartCoreTest, WheelEventZoomsTheView)
{
    chart.SetAxisValues(LinearAxisValues(20));
    chart.SetPanels(OnePanel(1, 20));
    chart.Paint(canvas, bounds);

    chart.OnWheel(ui::WheelEvent{ ui::Point{ 400.0f, 300.0f }, 1.0f, {} });

    EXPECT_TRUE(chart.Interaction().IsZoomed());
}

TEST_F(ChartCoreTest, DoubleClickResetsTheZoom)
{
    chart.SetAxisValues(LinearAxisValues(20));
    chart.SetPanels(OnePanel(1, 20));
    chart.Paint(canvas, bounds);

    chart.OnWheel(ui::WheelEvent{ ui::Point{ 400.0f, 300.0f }, 1.0f, {} });
    chart.OnMouseDoubleClick(ui::MouseEvent{ ui::Point{ 400.0f, 300.0f }, ui::MouseButton::Left, {} });

    EXPECT_FALSE(chart.Interaction().IsZoomed());
}

TEST_F(ChartCoreTest, RepaintIsRequestedOnInteraction)
{
    ui::backend::recording::RecordingPaintedHost host{ chart };

    chart.SetAxisValues(LinearAxisValues(20));

    EXPECT_GT(host.InvalidateCount(), 0u);
}

TEST_F(FrequencyChartCoreTest, NonPositiveFrequenciesAreDroppedFromThePolyline)
{
    chart.SetAxisValues(std::vector<float>{ 0.0f, 10.0f, 100.0f, 1000.0f });
    chart.SetPanels(OnePanel(1, 4));
    chart.Paint(canvas, bounds);

    for (const auto& command : canvas.Commands())
    {
        if (command.kind == CommandKind::DrawPolyline)
        {
            EXPECT_EQ(command.points.size(), 3u);
        }
    }
}

TEST_F(FrequencyChartCoreTest, TheFrequencyAxisTitleIsDrawn)
{
    chart.SetAxisValues(std::vector<float>{ 10.0f, 100.0f, 1000.0f });
    chart.SetPanels(OnePanel(1, 3));
    chart.Paint(canvas, bounds);

    EXPECT_THAT(canvas.Texts(), ::testing::Contains("Frequency (Hz)"));
}

TEST_F(FrequencyChartCoreTest, DecadeGridLinesAreDrawn)
{
    chart.SetAxisValues(std::vector<float>{ 10.0f, 100.0f, 1000.0f });
    chart.SetPanels(OnePanel(1, 3));
    chart.Paint(canvas, bounds);

    EXPECT_GT(canvas.CountOf(CommandKind::DrawLine), 0u);
}

TEST_F(FrequencyChartCoreTest, TickLabelsCarryTheKilohertzSuffix)
{
    chart.SetAxisValues(std::vector<float>{ 10.0f, 100.0f, 1000.0f });
    chart.SetPanels(OnePanel(1, 3));
    chart.Paint(canvas, bounds);

    EXPECT_THAT(canvas.Texts(), ::testing::Contains("1k"));
}

TEST_F(FrequencyChartCoreTest, TheSameEngineServesBothAxes)
{
    chart.SetAxisValues(std::vector<float>{ 10.0f, 100.0f, 1000.0f });
    chart.SetPanels(OnePanel(2, 3));
    chart.Paint(canvas, bounds);

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawPolyline), 2u);
    EXPECT_EQ(canvas.CountOf(CommandKind::SetClip), 2u);
}

// The crosshair and its readout were dropped when the two chart widgets were merged: the line was
// ported, the tooltip was not, leaving NearestSampleIndex, FormatCursorValue and DrawRoundedRect
// with no caller. These tests pin the restored behaviour against the original widget's.
TEST_F(ChartCoreTest, HoveringDrawsBothCrosshairLines)
{
    HoverAt(insidePlot);

    EXPECT_EQ(CrosshairLineCount(canvas), 2u);
}

TEST_F(ChartCoreTest, NoCrosshairIsDrawnBeforeTheCursorEnters)
{
    chart.SetAxisValues(LinearAxisValues(20));
    chart.SetPanels(OnePanel(1, 20));
    chart.Paint(canvas, bounds);

    EXPECT_EQ(CrosshairLineCount(canvas), 0u);
    EXPECT_EQ(ReadoutBackground(canvas), nullptr);
}

TEST_F(ChartCoreTest, LeavingHidesTheCrosshair)
{
    HoverAt(insidePlot);
    chart.OnMouseLeave();

    canvas.Clear();
    chart.Paint(canvas, bounds);

    EXPECT_EQ(CrosshairLineCount(canvas), 0u);
    EXPECT_EQ(ReadoutBackground(canvas), nullptr);
}

TEST_F(ChartCoreTest, HoveringOutsideThePlotAreaDrawsNoCrosshair)
{
    HoverAt(ui::Point{ 10.0f, 300.0f });

    EXPECT_EQ(CrosshairLineCount(canvas), 0u);
    EXPECT_EQ(ReadoutBackground(canvas), nullptr);
}

TEST_F(ChartCoreTest, TheReadoutNamesTheAxisValueAndEverySeries)
{
    HoverAt(insidePlot, 2);

    EXPECT_THAT(canvas.Texts(), ::testing::Contains(::testing::StartsWith("t = ")));
    EXPECT_THAT(canvas.Texts(), ::testing::Contains(::testing::StartsWith("series0 = ")));
    EXPECT_THAT(canvas.Texts(), ::testing::Contains(::testing::StartsWith("series1 = ")));
}

// 441px is (441-65)/715 of the way across a [0, 19] view, which is 9.99 - so the readout must
// report sample 10, not the 9 a truncating lookup would give.
TEST_F(ChartCoreTest, TheReadoutReportsTheNearestSampleRatherThanTheCursorPosition)
{
    HoverAt(insidePlot);

    EXPECT_THAT(canvas.Texts(), ::testing::Contains("series0 = 1.000"));
}

TEST_F(ChartCoreTest, TheReadoutSitsOnATranslucentSurface)
{
    HoverAt(insidePlot);

    const auto* background = ReadoutBackground(canvas);
    ASSERT_NE(background, nullptr);

    const auto& theme = ui::theme::Current();
    EXPECT_EQ(background->brush.color, theme.Get(ui::theme::ColorRole::Surface).WithAlpha(230));
    EXPECT_EQ(background->pen.color, theme.Get(ui::theme::ColorRole::Neutral));
}

TEST_F(ChartCoreTest, TheReadoutFlipsLeftOfACursorNearTheRightEdge)
{
    HoverAt(ui::Point{ 775.0f, 300.0f });

    const auto* background = ReadoutBackground(canvas);
    ASSERT_NE(background, nullptr);
    EXPECT_LT(background->rect.Right(), 775.0f);
}

TEST_F(ChartCoreTest, TheReadoutDropsBelowACursorNearTheTop)
{
    HoverAt(ui::Point{ 441.0f, 20.0f });

    const auto* background = ReadoutBackground(canvas);
    ASSERT_NE(background, nullptr);
    EXPECT_GT(background->rect.Top(), 20.0f);
}

// The readout writes into fixed storage, so a panel with more series than it holds must stop
// filling rather than run off the end of the array.
TEST_F(ChartCoreTest, TheReadoutIsCappedAtItsFixedLineCount)
{
    HoverAt(insidePlot, 12);

    // The legend draws a bare "seriesN" for each of the twelve; only the readout writes "N = value".
    std::size_t readoutLines{ 0 };
    for (const auto& text : canvas.Texts())
        if (text.find(" = ") != std::string::npos)
            ++readoutLines;

    EXPECT_EQ(readoutLines, 9u);
}

TEST_F(ChartCoreTest, DraggingPansTheView)
{
    chart.SetAxisValues(LinearAxisValues(20));
    chart.SetPanels(OnePanel(1, 20));
    chart.Paint(canvas, bounds);
    chart.OnWheel(ui::WheelEvent{ insidePlot, 1.0f, {} });

    const auto before = chart.Interaction().ViewMinimum();

    chart.OnMousePress(ui::MouseEvent{ insidePlot, ui::MouseButton::Left, {} });
    chart.OnMouseMove(ui::MouseEvent{ ui::Point{ 300.0f, 300.0f }, ui::MouseButton::None, {} });

    EXPECT_GT(chart.Interaction().ViewMinimum(), before);
}

TEST_F(ChartCoreTest, ReleasingEndsThePan)
{
    chart.SetAxisValues(LinearAxisValues(20));
    chart.SetPanels(OnePanel(1, 20));
    chart.Paint(canvas, bounds);
    chart.OnWheel(ui::WheelEvent{ insidePlot, 1.0f, {} });

    chart.OnMousePress(ui::MouseEvent{ insidePlot, ui::MouseButton::Left, {} });
    chart.OnMouseRelease(ui::MouseEvent{ insidePlot, ui::MouseButton::Left, {} });

    const auto after = chart.Interaction().ViewMinimum();
    chart.OnMouseMove(ui::MouseEvent{ ui::Point{ 300.0f, 300.0f }, ui::MouseButton::None, {} });

    EXPECT_NEAR(chart.Interaction().ViewMinimum(), after, 1e-6f);
}

TEST_F(ChartCoreTest, APressWithTheRightButtonDoesNotPan)
{
    chart.SetAxisValues(LinearAxisValues(20));
    chart.SetPanels(OnePanel(1, 20));
    chart.Paint(canvas, bounds);
    chart.OnWheel(ui::WheelEvent{ insidePlot, 1.0f, {} });

    const auto before = chart.Interaction().ViewMinimum();

    chart.OnMousePress(ui::MouseEvent{ insidePlot, ui::MouseButton::Right, {} });
    chart.OnMouseMove(ui::MouseEvent{ ui::Point{ 300.0f, 300.0f }, ui::MouseButton::None, {} });

    EXPECT_NEAR(chart.Interaction().ViewMinimum(), before, 1e-6f);
}

TEST_F(FrequencyChartCoreTest, TheReadoutUsesTheFrequencyFormatter)
{
    std::vector<float> frequencies;
    for (std::size_t i = 1; i <= 100; ++i)
        frequencies.push_back(static_cast<float>(i) * 100.0f);

    chart.SetAxisValues(frequencies);
    chart.SetPanels(OnePanel(1, 100));
    chart.OnMouseMove(ui::MouseEvent{ ui::Point{ 441.0f, 300.0f }, ui::MouseButton::None, {} });
    chart.Paint(canvas, bounds);

    EXPECT_THAT(canvas.Texts(), ::testing::Contains(::testing::StartsWith("f = ")));
    EXPECT_THAT(canvas.Texts(), ::testing::Contains(::testing::AnyOf(::testing::HasSubstr(" Hz"), ::testing::HasSubstr(" kHz"))));
}

// Quirks carried over from the original widgets deliberately, so they need pinning: an empty
// series falls back to a +/-1 Y range rather than collapsing the panel.
TEST_F(ChartCoreTest, APanelWithNoDataFallsBackToAUnitRange)
{
    auto panels = OnePanel(1, 0);

    chart.SetAxisValues(LinearAxisValues(20));
    chart.SetPanels(std::move(panels));
    chart.Paint(canvas, bounds);

    EXPECT_THAT(canvas.Texts(), ::testing::Contains("-1.00"));
    EXPECT_THAT(canvas.Texts(), ::testing::Contains("1.00"));
}

// Above one sample per half-pixel the series is strided, and the original closed the trace with a
// segment to the final sample whenever the stride stepped over it.
TEST_F(ChartCoreTest, AStridedSeriesStillReachesItsFinalSample)
{
    chart.SetAxisValues(LinearAxisValues(3000));
    chart.SetPanels(OnePanel(1, 3000));
    chart.Paint(canvas, bounds);

    for (const auto& command : canvas.Commands())
    {
        if (command.kind == CommandKind::DrawPolyline)
        {
            EXPECT_EQ(command.points.size(), 1501u);
        }
    }
}

TEST_F(ChartCoreTest, TheReadoutClampsToTheFirstSampleAtTheLeftEdge)
{
    HoverAt(ui::Point{ 65.0f, 300.0f });

    EXPECT_THAT(canvas.Texts(), ::testing::Contains("series0 = 0.000"));
}

TEST_F(ChartCoreTest, TheReadoutClampsToTheLastSampleAtTheRightEdge)
{
    HoverAt(ui::Point{ 780.0f, 300.0f });

    EXPECT_THAT(canvas.Texts(), ::testing::Contains("series0 = 1.900"));
}

TEST_F(ChartCoreTest, ASeriesWithNoDataIsSkippedInTheReadout)
{
    auto panels = OnePanel(2, 20);
    panels[0].series[1].data.clear();

    chart.SetAxisValues(LinearAxisValues(20));
    chart.SetPanels(std::move(panels));
    chart.OnMouseMove(ui::MouseEvent{ insidePlot, ui::MouseButton::None, {} });
    chart.Paint(canvas, bounds);

    std::size_t readoutLines{ 0 };
    for (const auto& text : canvas.Texts())
        if (text.find(" = ") != std::string::npos)
            ++readoutLines;

    EXPECT_EQ(readoutLines, 2u);
}
