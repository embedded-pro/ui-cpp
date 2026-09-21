#include "ui/charts/LinearAxis.hpp"
#include "ui/charts/Log10Axis.hpp"
#include <array>
#include <gmock/gmock.h>

namespace
{
    class LinearAxisTest
        : public ::testing::Test
    {
    protected:
        ui::charts::LinearAxis axis{ ui::charts::LinearAxis::Time() };
        std::array<ui::charts::Tick, 32> ticks{};
        std::array<ui::charts::GridLine, 128> gridLines{};

        static constexpr float tolerance{ 1e-5f };
    };

    class Log10AxisTest
        : public ::testing::Test
    {
    protected:
        ui::charts::Log10Axis axis;
        std::array<ui::charts::Tick, 32> ticks{};
        std::array<ui::charts::GridLine, 128> gridLines{};

        static constexpr float tolerance{ 1e-4f };
    };
}

TEST_F(LinearAxisTest, ViewSpaceIsTheIdentity)
{
    EXPECT_NEAR(axis.ToView(4.2f), 4.2f, tolerance);
    EXPECT_NEAR(axis.FromView(4.2f), 4.2f, tolerance);
}

TEST_F(LinearAxisTest, EveryValueIsPlottable)
{
    EXPECT_TRUE(axis.IsPlottable(-1.0f));
    EXPECT_TRUE(axis.IsPlottable(0.0f));
}

TEST_F(LinearAxisTest, RangeIsAnchoredAtZeroNotTheFirstSample)
{
    const std::array<float, 3> values{ 2.0f, 3.0f, 4.0f };
    const auto range = axis.RangeFor(values);

    EXPECT_NEAR(range.minimum, 0.0f, tolerance);
    EXPECT_NEAR(range.maximum, 4.0f, tolerance);
}

TEST_F(LinearAxisTest, RangeOfEmptyDataIsDegenerate)
{
    const auto range = axis.RangeFor(std::span<const float>{});

    EXPECT_NEAR(range.minimum, 0.0f, tolerance);
    EXPECT_NEAR(range.maximum, 0.0f, tolerance);
}

TEST_F(LinearAxisTest, TicksAreEvenlySpacedInclusiveOfBothEnds)
{
    const auto count = axis.Ticks(ui::charts::AxisRange{ 0.0f, 1.0f }, ticks);

    ASSERT_EQ(count, 6u);
    EXPECT_NEAR(ticks[0].viewPosition, 0.0f, tolerance);
    EXPECT_NEAR(ticks[5].viewPosition, 1.0f, tolerance);
    EXPECT_EQ(ticks[0].Label(), "0.00");
    EXPECT_EQ(ticks[5].Label(), "1.00");
}

TEST_F(LinearAxisTest, TicksHonourTheOutputCapacity)
{
    std::array<ui::charts::Tick, 3> small{};
    EXPECT_EQ(axis.Ticks(ui::charts::AxisRange{ 0.0f, 1.0f }, small), 3u);
}

TEST_F(LinearAxisTest, GridLinesExcludeTheLeftEdge)
{
    const auto count = axis.GridLines(ui::charts::AxisRange{ 0.0f, 1.0f }, gridLines);

    ASSERT_EQ(count, 5u);
    EXPECT_GT(gridLines[0].viewPosition, 0.0f);
    EXPECT_TRUE(gridLines[0].major);
}

TEST_F(LinearAxisTest, GridLinesOfADegenerateRangeAreEmpty)
{
    EXPECT_EQ(axis.GridLines(ui::charts::AxisRange{ 1.0f, 1.0f }, gridLines), 0u);
}

TEST_F(LinearAxisTest, TitleAndCursorReadoutCarryUnits)
{
    EXPECT_EQ(axis.Title(), "Time (s)");

    std::array<char, 64> scratch{};
    const auto length = axis.FormatCursorValue(0.25f, scratch);

    EXPECT_EQ((std::string_view{ scratch.data(), length }), "t = 0.250 s");
}

TEST_F(Log10AxisTest, ViewSpaceIsLogarithmic)
{
    EXPECT_NEAR(axis.ToView(100.0f), 2.0f, tolerance);
    EXPECT_NEAR(axis.FromView(2.0f), 100.0f, 1e-2f);
}

TEST_F(Log10AxisTest, RoundTripsThroughViewSpace)
{
    EXPECT_NEAR(axis.FromView(axis.ToView(440.0f)), 440.0f, 1e-1f);
}

TEST_F(Log10AxisTest, NonPositiveValuesAreNotPlottable)
{
    EXPECT_FALSE(axis.IsPlottable(0.0f));
    EXPECT_FALSE(axis.IsPlottable(-1.0f));
    EXPECT_TRUE(axis.IsPlottable(1e-6f));
}

TEST_F(Log10AxisTest, RangeUsesLogOfTheDataExtremes)
{
    const std::array<float, 3> values{ 10.0f, 100.0f, 1000.0f };
    const auto range = axis.RangeFor(values);

    EXPECT_NEAR(range.minimum, 1.0f, tolerance);
    EXPECT_NEAR(range.maximum, 3.0f, tolerance);
}

TEST_F(Log10AxisTest, NonPositiveMinimumIsClampedToUnity)
{
    const std::array<float, 3> values{ 0.0f, 10.0f, 100.0f };
    const auto range = axis.RangeFor(values);

    EXPECT_NEAR(range.minimum, 0.0f, tolerance);
    EXPECT_NEAR(range.maximum, 2.0f, tolerance);
}

TEST_F(Log10AxisTest, TicksAreTheEndpointsPlusInteriorDecades)
{
    const auto count = axis.Ticks(ui::charts::AxisRange{ 0.0f, 3.0f }, ticks);

    ASSERT_EQ(count, 4u);
    EXPECT_NEAR(ticks[0].viewPosition, 0.0f, tolerance);
    EXPECT_NEAR(ticks[1].viewPosition, 1.0f, tolerance);
    EXPECT_NEAR(ticks[2].viewPosition, 2.0f, tolerance);
    EXPECT_NEAR(ticks[3].viewPosition, 3.0f, tolerance);
}

TEST_F(Log10AxisTest, MagnitudeLabelsUseAKilohertzSuffix)
{
    std::array<char, 24> scratch{};

    auto length = ui::charts::Log10Axis::FormatMagnitude(500.0f, scratch);
    EXPECT_EQ((std::string_view{ scratch.data(), length }), "500");

    length = ui::charts::Log10Axis::FormatMagnitude(2000.0f, scratch);
    EXPECT_EQ((std::string_view{ scratch.data(), length }), "2k");
}

TEST_F(Log10AxisTest, GridLinesSubdivideEachDecade)
{
    const auto count = axis.GridLines(ui::charts::AxisRange{ 1.0f, 2.0f }, gridLines);

    ASSERT_GT(count, 0u);
    for (std::size_t i = 0; i < count; ++i)
    {
        EXPECT_GT(gridLines[i].viewPosition, 1.0f);
        EXPECT_LT(gridLines[i].viewPosition, 2.0f);
    }
}

TEST_F(Log10AxisTest, DecadeBoundariesAreMajorGridLines)
{
    const auto count = axis.GridLines(ui::charts::AxisRange{ 0.5f, 2.5f }, gridLines);

    auto majors = 0;
    for (std::size_t i = 0; i < count; ++i)
        if (gridLines[i].major)
            ++majors;

    EXPECT_EQ(majors, 2);
}

TEST_F(Log10AxisTest, CursorReadoutSwitchesUnitAtAKilohertz)
{
    std::array<char, 64> scratch{};

    auto length = axis.FormatCursorValue(250.0f, scratch);
    EXPECT_EQ((std::string_view{ scratch.data(), length }), "f = 250.0 Hz");

    length = axis.FormatCursorValue(2500.0f, scratch);
    EXPECT_EQ((std::string_view{ scratch.data(), length }), "f = 2.50 kHz");
}

TEST_F(Log10AxisTest, AnEmptyDataSetHasNoRange)
{
    const auto range = axis.RangeFor(std::span<const float>{});

    EXPECT_NEAR(range.Span(), 0.0f, tolerance);
}

// A degenerate view has no decades to walk, and a caller-owned buffer may be too small to hold
// what a full view would emit; neither may run off the end of the span.
TEST_F(Log10AxisTest, ADegenerateViewProducesNoTicksOrGridLines)
{
    const ui::charts::AxisRange collapsed{ 2.0f, 2.0f };

    EXPECT_EQ(axis.Ticks(collapsed, ticks), 0u);
    EXPECT_EQ(axis.GridLines(collapsed, gridLines), 0u);
}

TEST_F(Log10AxisTest, TicksAndGridLinesStopAtTheEndOfTheirBuffer)
{
    const ui::charts::AxisRange wide{ 0.0f, 6.0f };

    EXPECT_EQ(axis.Ticks(wide, std::span<ui::charts::Tick>{ ticks.data(), 2 }), 2u);
    EXPECT_EQ(axis.GridLines(wide, std::span<ui::charts::GridLine>{ gridLines.data(), 4 }), 4u);
}
