#include "ui/backend/qt/QtCanvas.hpp"
#include "ui/backend/qt/test/QtTestSupport.hpp"
#include "ui/backend/recording/RecordingCanvas.hpp"
#include "ui/charts/ChartCore.hpp"
#include "ui/charts/LinearAxis.hpp"
#include "ui/charts/Log10Axis.hpp"
#include <QPainter>
#include <cmath>
#include <gmock/gmock.h>

namespace
{
    using ui::backend::qt::test::HasInk;
    using ui::backend::recording::CommandKind;

    constexpr QRgb background{ 0xFFFFFFFFu };
    constexpr ui::Rect bounds{ 0.0f, 0.0f, 640.0f, 480.0f };

    std::vector<ui::charts::ChartPanel> OnePanel(std::size_t sampleCount)
    {
        ui::charts::Series series;
        series.name = "magnitude";
        series.color = ui::theme::Light().Series(0);

        for (std::size_t i = 0; i < sampleCount; ++i)
            series.data.push_back(std::sin(static_cast<float>(i) * 0.05f));

        ui::charts::ChartPanel panel;
        panel.title = "Step response";
        panel.yAxisLabel = "Amplitude";
        panel.series.push_back(std::move(series));

        return { std::move(panel) };
    }

    std::vector<float> Ramp(std::size_t count, float step)
    {
        std::vector<float> values;
        for (std::size_t i = 0; i < count; ++i)
            values.push_back(step + static_cast<float>(i) * step);

        return values;
    }

    QRect Around(ui::Point point, int radius)
    {
        return QRect{ static_cast<int>(point.x) - radius, static_cast<int>(point.y) - radius, 2 * radius + 1, 2 * radius + 1 };
    }

    // The gate the whole abstraction rests on: the identical scene, produced by the identical
    // portable code, driven into the recording backend and into QPainter. What the recording says
    // was drawn has to be what Qt actually painted, and neither backend may need the engine to
    // behave differently for it.
    class QtConformanceTest
        : public ::testing::Test
    {
    protected:
        QtConformanceTest()
        {
            image.fill(background);
        }

        void PaintThroughBoth(ui::charts::ChartCore& chart)
        {
            chart.Paint(recording, bounds);

            QPainter painter{ &image };
            ui::backend::qt::QtCanvas canvas{ painter };
            chart.Paint(canvas, bounds);
            painter.end();
        }

        [[nodiscard]] const ui::backend::recording::Command* FirstOf(CommandKind kind) const
        {
            for (const auto& command : recording.Commands())
                if (command.kind == kind)
                    return &command;

            return nullptr;
        }

        ui::backend::recording::RecordingCanvas recording;
        QImage image{ 640, 480, QImage::Format_ARGB32 };

        ui::charts::LinearAxis timeAxis{ ui::charts::LinearAxis::Time() };
        ui::charts::ChartCore timeChart{ timeAxis, ui::charts::ChartConfig{} };

        ui::charts::Log10Axis frequencyAxis;
        ui::charts::ChartCore frequencyChart{ frequencyAxis, ui::charts::ChartConfig{ 1, 2 } };
    };
}

TEST_F(QtConformanceTest, TimeChartRendersThroughBothBackends)
{
    timeChart.SetAxisValues(Ramp(256, 0.01f));
    timeChart.SetPanels(OnePanel(256));

    PaintThroughBoth(timeChart);

    EXPECT_FALSE(recording.Commands().empty());
    EXPECT_TRUE(HasInk(image, image.rect(), background));
}

TEST_F(QtConformanceTest, FrequencyChartRendersThroughBothBackends)
{
    frequencyChart.SetAxisValues(Ramp(256, 10.0f));
    frequencyChart.SetPanels(OnePanel(256));

    PaintThroughBoth(frequencyChart);

    EXPECT_FALSE(recording.Commands().empty());
    EXPECT_TRUE(HasInk(image, image.rect(), background));
}

TEST_F(QtConformanceTest, AxisLinesLandAtTheRecordedCoordinates)
{
    timeChart.SetAxisValues(Ramp(256, 0.01f));
    timeChart.SetPanels(OnePanel(256));

    PaintThroughBoth(timeChart);

    const auto* axisLine = FirstOf(CommandKind::DrawLine);
    ASSERT_NE(axisLine, nullptr);

    const ui::Point midpoint{ (axisLine->from.x + axisLine->to.x) * 0.5f, (axisLine->from.y + axisLine->to.y) * 0.5f };
    EXPECT_TRUE(HasInk(image, Around(midpoint, 2), background));
}

TEST_F(QtConformanceTest, SeriesTraceLandsAtTheRecordedPoints)
{
    timeChart.SetAxisValues(Ramp(256, 0.01f));
    timeChart.SetPanels(OnePanel(256));

    PaintThroughBoth(timeChart);

    const auto* polyline = FirstOf(CommandKind::DrawPolyline);
    ASSERT_NE(polyline, nullptr);
    ASSERT_GT(polyline->points.size(), 16u);

    for (std::size_t i = 0; i < polyline->points.size(); i += 16)
        EXPECT_TRUE(HasInk(image, Around(polyline->points[i], 2), background)) << "no ink at recorded sample " << i;
}

TEST_F(QtConformanceTest, TheSeriesTraceIsOnePolylineNotOneLinePerSample)
{
    timeChart.SetAxisValues(Ramp(256, 0.01f));
    timeChart.SetPanels(OnePanel(256));

    PaintThroughBoth(timeChart);

    EXPECT_EQ(recording.CountOf(CommandKind::DrawPolyline), 1u);
    EXPECT_LT(recording.CountOf(CommandKind::DrawLine), 32u);
}

TEST_F(QtConformanceTest, EveryRecordedPrimitiveStaysInsideTheWidgetBounds)
{
    timeChart.SetAxisValues(Ramp(256, 0.01f));
    timeChart.SetPanels(OnePanel(256));

    PaintThroughBoth(timeChart);

    for (const auto& command : recording.Commands())
    {
        for (const auto& point : command.points)
        {
            EXPECT_GE(point.x, bounds.Left());
            EXPECT_LE(point.x, bounds.Right());
            EXPECT_GE(point.y, bounds.Top());
            EXPECT_LE(point.y, bounds.Bottom());
        }
    }
}

// What "the two backends see the same scene" actually reduces to: the engine emits an
// identical command stream regardless of which Canvas it is handed, so a QPainter-only
// behaviour cannot leak back into the portable layer.
TEST_F(QtConformanceTest, TheCommandStreamIsUnchangedByHavingRenderedThroughQt)
{
    timeChart.SetAxisValues(Ramp(256, 0.01f));
    timeChart.SetPanels(OnePanel(256));

    timeChart.Paint(recording, bounds);
    const auto before = recording.Commands().size();

    QPainter painter{ &image };
    ui::backend::qt::QtCanvas canvas{ painter };
    timeChart.Paint(canvas, bounds);
    painter.end();

    recording.Clear();
    timeChart.Paint(recording, bounds);

    EXPECT_EQ(recording.Commands().size(), before);
}

TEST_F(QtConformanceTest, ClippingKeepsTheTraceInsideTheRecordedPlotArea)
{
    timeChart.SetAxisValues(Ramp(256, 0.01f));
    timeChart.SetPanels(OnePanel(256));

    PaintThroughBoth(timeChart);

    const auto* clip = FirstOf(CommandKind::SetClip);
    ASSERT_NE(clip, nullptr);

    const auto series = ui::theme::Light().Series(0);
    const auto seriesPixel = qRgb(series.red, series.green, series.blue);

    for (auto y = 0; y < image.height(); ++y)
        for (auto x = 0; x < image.width(); ++x)
            if (image.pixel(x, y) == seriesPixel)
            {
                EXPECT_GE(static_cast<float>(x), clip->rect.Left() - 1.0f);
                EXPECT_LE(static_cast<float>(x), clip->rect.Right() + 1.0f);
            }
}

TEST_F(QtConformanceTest, BothBackendsAgreeOnASuppressedOutline)
{
    QPainter painter{ &image };
    ui::backend::qt::QtCanvas canvas{ painter };

    for (auto* target : std::initializer_list<ui::Canvas*>{ &recording, &canvas })
    {
        target->SetPen(ui::Pen{ ui::colors::black, 8.0f, ui::LineStyle::None });
        target->SetBrush(ui::Brush{ ui::Color::Rgb(0x2980B9) });
        target->DrawEllipse(ui::Point{ 320.0f, 240.0f }, 40.0f, 40.0f);
    }

    painter.end();

    EXPECT_EQ(recording.Commands().back().pen.style, ui::LineStyle::None);
    EXPECT_EQ(image.pixel(320, 240), qRgb(0x29, 0x80, 0xB9));
    EXPECT_FALSE(HasInk(image, QRect{ 0, 0, 640, 160 }, background));
}

TEST_F(QtConformanceTest, BothBackendsReportLineHeightAsTheHeightTheyMeasure)
{
    QPainter painter{ &image };
    ui::backend::qt::QtCanvas canvas{ painter };

    const ui::FontSpec font{ ui::FontFamily::UiDefault, 10, false, false };
    recording.SetFont(font);
    canvas.SetFont(font);

    EXPECT_NEAR(recording.MeasureText("readout").height, recording.LineHeight(), 1e-3f);
    EXPECT_NEAR(canvas.MeasureText("readout").height, canvas.LineHeight(), 1e-3f);

    painter.end();
}
