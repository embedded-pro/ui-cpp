#include "ui/backend/recording/RecordingCanvas.hpp"
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

    class ChartCoreTest
        : public ::testing::Test
    {
    protected:
        ui::backend::recording::RecordingCanvas canvas;
        ui::charts::LinearAxis axis{ ui::charts::LinearAxis::Time() };
        ui::charts::ChartCore chart{ axis, ui::charts::ChartConfig{} };

        static constexpr ui::Rect bounds{ 0.0f, 0.0f, 800.0f, 600.0f };
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
    auto repaints = 0;
    chart.onRepaintRequested = [&repaints]
    {
        ++repaints;
    };

    chart.SetAxisValues(LinearAxisValues(20));

    EXPECT_GT(repaints, 0);
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
