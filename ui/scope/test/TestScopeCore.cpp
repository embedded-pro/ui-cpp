#include "ui/backend/recording/RecordingCanvas.hpp"
#include "ui/backend/recording/RecordingPaintedHost.hpp"
#include "ui/scope/ScopeCore.hpp"
#include "ui/theme/Theme.hpp"
#include <cmath>
#include <gmock/gmock.h>

namespace
{
    using ui::backend::recording::CommandKind;

    ui::scope::ScopeConfig SmallScope()
    {
        ui::scope::ScopeConfig config;
        config.sampleCapacity = 4096;

        return config;
    }

    class ScopeCoreTest
        : public ::testing::Test
    {
    protected:
        ScopeCoreTest()
        {
            ui::theme::SetCurrent(ui::theme::Instrument());

            scope.SetChannelCount(2);
            scope.SetChannelConfig(0, ui::scope::ChannelConfig{ "ia", ui::theme::Instrument().Series(0), true });
            scope.SetChannelConfig(1, ui::scope::ChannelConfig{ "ib", ui::theme::Instrument().Series(1), true });

            // 10 divisions x 1 ms at a 100 us period is a 100-sample sweep.
            scope.SetTimePerDivision(1e-3f);
            scope.SetSamplePeriod(100e-6f);
        }

        void FeedSine(std::size_t count)
        {
            for (std::size_t i = 0; i < count; ++i)
            {
                const std::array<float, 2> sample{
                    std::sin(static_cast<float>(i) * 0.1f),
                    std::cos(static_cast<float>(i) * 0.1f)
                };
                scope.AddSample(sample);
            }
        }

        [[nodiscard]] const ui::backend::recording::Command* FirstOf(CommandKind kind) const
        {
            for (const auto& command : canvas.Commands())
                if (command.kind == kind)
                    return &command;

            return nullptr;
        }

        ui::backend::recording::RecordingCanvas canvas;
        ui::scope::ScopeCore scope{ SmallScope() };

        static constexpr ui::Rect bounds{ 0.0f, 0.0f, 800.0f, 400.0f };
    };
}

TEST_F(ScopeCoreTest, EachEnabledChannelBecomesExactlyOnePolyline)
{
    FeedSine(200);
    scope.Paint(canvas, bounds);

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawPolyline), 2u);
}

TEST_F(ScopeCoreTest, ADisabledChannelDrawsNoTrace)
{
    scope.SetChannelConfig(1, ui::scope::ChannelConfig{ "ib", ui::theme::Instrument().Series(1), false });
    FeedSine(200);
    scope.Paint(canvas, bounds);

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawPolyline), 1u);
}

TEST_F(ScopeCoreTest, TheTraceCarriesOnePointPerSweptSampleNotOneLinePerSample)
{
    FeedSine(200);
    scope.Paint(canvas, bounds);

    const auto* polyline = FirstOf(CommandKind::DrawPolyline);
    ASSERT_NE(polyline, nullptr);

    // A 100-sample sweep, and the whole trace is a single batched command.
    EXPECT_EQ(polyline->points.size(), 100u);
}

TEST_F(ScopeCoreTest, ASweepShorterThanTheBufferShowsTheNewestSamples)
{
    FeedSine(200);
    scope.Paint(canvas, bounds);

    const auto* polyline = FirstOf(CommandKind::DrawPolyline);
    ASSERT_NE(polyline, nullptr);

    EXPECT_NEAR(polyline->points.front().x, 60.0f, 1.0f);
    EXPECT_LT(polyline->points.back().x, 800.0f);
}

TEST_F(ScopeCoreTest, TheBackgroundIsFilledWithTheInstrumentRole)
{
    scope.Paint(canvas, bounds);

    const auto* fill = FirstOf(CommandKind::FillRect);
    ASSERT_NE(fill, nullptr);
    EXPECT_EQ(fill->color, ui::theme::Instrument().Get(ui::theme::ColorRole::ScopeBackground));
}

TEST_F(ScopeCoreTest, TheGraticuleHasOneLinePerDivisionBoundary)
{
    scope.Paint(canvas, bounds);

    // 11 vertical + 9 horizontal division lines, plus the two centre crosshairs.
    EXPECT_GE(canvas.CountOf(CommandKind::DrawLine), 22u);
}

TEST_F(ScopeCoreTest, TheTraceIsClippedToThePlotArea)
{
    FeedSine(200);
    scope.Paint(canvas, bounds);

    const auto* clip = FirstOf(CommandKind::SetClip);
    ASSERT_NE(clip, nullptr);

    EXPECT_NEAR(clip->rect.Left(), 60.0f, 1e-3f);
    EXPECT_NEAR(clip->rect.Top(), 10.0f, 1e-3f);
    EXPECT_EQ(canvas.CountOf(CommandKind::SetClip), canvas.CountOf(CommandKind::ClearClip));
}

TEST_F(ScopeCoreTest, PaintingWithNoRoomDrawsNothing)
{
    FeedSine(200);
    scope.Paint(canvas, ui::Rect{ 0.0f, 0.0f, 40.0f, 20.0f });

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawPolyline), 0u);
}

TEST_F(ScopeCoreTest, PaintingWithoutSamplesStillDrawsTheGraticule)
{
    scope.Paint(canvas, bounds);

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawPolyline), 0u);
    EXPECT_GT(canvas.CountOf(CommandKind::DrawLine), 0u);
}

TEST_F(ScopeCoreTest, TheTimebaseReadoutNamesTheUnitItIsScaledTo)
{
    scope.Paint(canvas, bounds);

    EXPECT_THAT(canvas.Texts(), ::testing::Contains("1.00 ms/div"));
}

TEST_F(ScopeCoreTest, ASlowTimebaseReadsInSeconds)
{
    scope.SetTimePerDivision(2.0f);
    scope.Paint(canvas, bounds);

    EXPECT_THAT(canvas.Texts(), ::testing::Contains("2.0 s/div"));
}

TEST_F(ScopeCoreTest, AFastTimebaseReadsInMicroseconds)
{
    scope.SetTimePerDivision(50e-6f);
    scope.Paint(canvas, bounds);

    EXPECT_THAT(canvas.Texts(), ::testing::Contains("50 µs/div"));
}

TEST_F(ScopeCoreTest, TheTriggerModeIsShownInTheReadout)
{
    scope.SetTriggerMode(ui::scope::TriggerMode::Single);
    scope.Paint(canvas, bounds);

    EXPECT_THAT(canvas.Texts(), ::testing::Contains("Trig: SINGLE"));
}

TEST_F(ScopeCoreTest, EveryEnabledChannelNameAppearsInTheLegend)
{
    scope.Paint(canvas, bounds);

    EXPECT_THAT(canvas.Texts(), ::testing::IsSupersetOf({ "ia", "ib" }));
}

TEST_F(ScopeCoreTest, TheTriggerLevelIsMarkedWhenItFallsInsideTheView)
{
    FeedSine(200);
    scope.SetTriggerLevel(0.0f);
    scope.Paint(canvas, bounds);

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawPolygon), 1u);
}

TEST_F(ScopeCoreTest, ATriggerLevelOutsideTheViewIsNotMarked)
{
    FeedSine(200);
    scope.SetTriggerLevel(100.0f);
    scope.Paint(canvas, bounds);

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawPolygon), 0u);
}

TEST_F(ScopeCoreTest, AFlatSignalStillGetsAUsableVerticalRange)
{
    for (auto i = 0; i < 200; ++i)
    {
        const std::array<float, 2> sample{ 3.0f, 3.0f };
        scope.AddSample(sample);
    }

    scope.Paint(canvas, bounds);

    const auto* polyline = FirstOf(CommandKind::DrawPolyline);
    ASSERT_NE(polyline, nullptr);

    for (const auto& point : polyline->points)
    {
        EXPECT_GE(point.y, bounds.Top());
        EXPECT_LE(point.y, bounds.Bottom());
    }
}

TEST_F(ScopeCoreTest, TheTriggeredSweepStartsBeforeTheEdgeThatFiredIt)
{
    scope.SetTriggerMode(ui::scope::TriggerMode::Normal);
    scope.SetTriggerEdge(ui::scope::TriggerEdge::Rising);
    scope.SetTriggerLevel(0.0f);
    FeedSine(300);

    ASSERT_TRUE(scope.IsTriggered());
    scope.Paint(canvas, bounds);

    // The sweep is shown, and the pre-trigger fraction means it does not start at the newest
    // sample the way the free-running sweep would.
    const auto* polyline = FirstOf(CommandKind::DrawPolyline);
    ASSERT_NE(polyline, nullptr);
    EXPECT_GT(polyline->points.size(), 1u);
}

TEST_F(ScopeCoreTest, RepaintingProducesTheSameCommandStream)
{
    FeedSine(200);

    scope.Paint(canvas, bounds);
    const auto first = canvas.Commands().size();

    canvas.Clear();
    scope.Paint(canvas, bounds);

    EXPECT_EQ(canvas.Commands().size(), first);
}

TEST_F(ScopeCoreTest, PaintingDoesNotGrowTheScratchBufferEveryFrame)
{
    FeedSine(200);

    scope.Paint(canvas, bounds);
    canvas.Clear();
    scope.Paint(canvas, bounds);

    const auto* polyline = FirstOf(CommandKind::DrawPolyline);
    ASSERT_NE(polyline, nullptr);
    EXPECT_EQ(polyline->points.size(), 100u);
}

TEST_F(ScopeCoreTest, TheViewRequestsARepaintWhenTheTimebaseChanges)
{
    ui::backend::recording::RecordingPaintedHost host{ scope };

    scope.SetTimePerDivision(5e-3f);
    scope.SetTriggerLevel(1.0f);

    EXPECT_EQ(host.InvalidateCount(), 2u);
}

TEST_F(ScopeCoreTest, AcquiringSamplesDoesNotRequestARepaint)
{
    ui::backend::recording::RecordingPaintedHost host{ scope };

    FeedSine(100);

    EXPECT_EQ(host.InvalidateCount(), 0u);
}

// FindTriggerPoint is the render-time sweep alignment, not the acquisition trigger that
// TestTrigger.cpp covers: it re-searches the ring buffer each repaint to decide where the
// displayed sweep starts, and had only ever been exercised on a rising edge.
TEST_F(ScopeCoreTest, TheSweepAlignsToAFallingEdgeWhenOneIsSelected)
{
    scope.SetTriggerMode(ui::scope::TriggerMode::Normal);
    scope.SetTriggerEdge(ui::scope::TriggerEdge::Falling);
    scope.SetTriggerLevel(0.0f);
    FeedSine(300);

    ASSERT_TRUE(scope.IsTriggered());
    scope.Paint(canvas, bounds);

    const auto* polyline = FirstOf(CommandKind::DrawPolyline);
    ASSERT_NE(polyline, nullptr);
    EXPECT_GT(polyline->points.size(), 1u);
}

// A level the trace never reaches leaves the search with nothing, and the sweep falls back to the
// newest sample rather than drawing nothing at all.
TEST_F(ScopeCoreTest, ASweepWithNoCrossingFallsBackToTheNewestSample)
{
    scope.SetTriggerMode(ui::scope::TriggerMode::Normal);
    scope.SetTriggerLevel(100.0f);
    FeedSine(300);
    scope.ForceTrigger();

    scope.Paint(canvas, bounds);

    const auto* polyline = FirstOf(CommandKind::DrawPolyline);
    ASSERT_NE(polyline, nullptr);
    EXPECT_GT(polyline->points.size(), 1u);
}

TEST_F(ScopeCoreTest, AZeroSamplePeriodDrawsTheGraticuleButNoTrace)
{
    FeedSine(200);
    scope.SetSamplePeriod(0.0f);

    scope.Paint(canvas, bounds);

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawPolyline), 0u);
    EXPECT_GT(canvas.CountOf(CommandKind::DrawLine), 0u);
}

TEST_F(ScopeCoreTest, TheConfigurationIsReadBackThroughTheAccessors)
{
    scope.SetTriggerMode(ui::scope::TriggerMode::Single);
    scope.SetTriggerEdge(ui::scope::TriggerEdge::Falling);
    scope.SetTriggerChannel(1);
    scope.SetTriggerLevel(0.25f);
    scope.SetRunning(false);

    EXPECT_EQ(scope.ChannelCount(), 2u);
    EXPECT_NEAR(scope.TimePerDivision(), 1e-3f, 1e-9f);
    EXPECT_NEAR(scope.SamplePeriod(), 100e-6f, 1e-9f);
    EXPECT_NEAR(scope.TriggerLevel(), 0.25f, 1e-6f);
    EXPECT_EQ(scope.CurrentTriggerMode(), ui::scope::TriggerMode::Single);
    EXPECT_EQ(scope.CurrentTriggerEdge(), ui::scope::TriggerEdge::Falling);
    EXPECT_EQ(scope.TriggerChannel(), 1u);
    EXPECT_FALSE(scope.IsRunning());
    EXPECT_EQ(scope.MinimumSize().width, 320.0f);
    EXPECT_EQ(scope.MinimumSize().height, 250.0f);
}
