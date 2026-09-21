#include "ui/backend/recording/RecordingCanvas.hpp"
#include "ui/backend/recording/RecordingPaintedHost.hpp"
#include "ui/widgets/HexagonCore.hpp"
#include <cmath>
#include <gmock/gmock.h>
#include <numbers>

namespace
{
    using ui::backend::recording::CommandKind;

    constexpr float pi{ std::numbers::pi_v<float> };
    constexpr ui::Rect bounds{ 0.0f, 0.0f, 600.0f, 600.0f };

    ui::Point UnitAt(float degrees)
    {
        const auto radians = degrees * pi / 180.0f;
        return ui::Point{ std::cos(radians), std::sin(radians) };
    }

    class HexagonSectorTest
        : public ::testing::Test
    {
    };

    class HexagonTickStepTest
        : public ::testing::Test
    {
    };

    class HexagonPeakTest
        : public ::testing::Test
    {
    protected:
        ui::widgets::HexagonCore hexagon;
    };

    class HexagonCoreTest
        : public ::testing::Test
    {
    protected:
        HexagonCoreTest()
        {
            hexagon.SetDcLinkVolts(24.0f);
        }

        [[nodiscard]] std::size_t CountOf(CommandKind kind) const
        {
            return canvas.CountOf(kind);
        }

        ui::backend::recording::RecordingCanvas canvas;
        ui::widgets::HexagonCore hexagon;
    };
}

TEST_F(HexagonSectorTest, AVectorTooShortToHaveADirectionHasNoSector)
{
    EXPECT_EQ(ui::widgets::HexagonCore::SectorOf(0.0f, 0.0f), 0);
    EXPECT_EQ(ui::widgets::HexagonCore::SectorOf(1e-7f, 1e-7f), 0);
}

TEST_F(HexagonSectorTest, EachSixtyDegreeWedgeIsItsOwnSector)
{
    const std::array<std::pair<float, int>, 12> expected{
        std::pair{ 0.0f, 1 }, std::pair{ 30.0f, 1 },
        std::pair{ 60.0f, 2 }, std::pair{ 90.0f, 2 },
        std::pair{ 120.0f, 3 }, std::pair{ 150.0f, 3 },
        std::pair{ 180.0f, 4 }, std::pair{ 210.0f, 4 },
        std::pair{ 240.0f, 5 }, std::pair{ 270.0f, 5 },
        std::pair{ 300.0f, 6 }, std::pair{ 330.0f, 6 }
    };

    for (const auto& [degrees, sector] : expected)
    {
        const auto point = UnitAt(degrees);
        EXPECT_EQ(ui::widgets::HexagonCore::SectorOf(point.x, point.y), sector) << "at " << degrees << " degrees";
    }
}

TEST_F(HexagonSectorTest, ANegativeAngleWrapsIntoTheUpperSectors)
{
    const auto justBelowAxis = UnitAt(-1.0f);
    const auto wellBelowAxis = UnitAt(-90.0f);

    EXPECT_EQ(ui::widgets::HexagonCore::SectorOf(justBelowAxis.x, justBelowAxis.y), 6);
    EXPECT_EQ(ui::widgets::HexagonCore::SectorOf(wellBelowAxis.x, wellBelowAxis.y), 5);
}

TEST_F(HexagonTickStepTest, TheStepIsAOneTwoOrFiveTimesAPowerOfTen)
{
    const std::array<std::pair<float, float>, 7> expected{
        std::pair{ 6.0f, 1.0f },
        std::pair{ 12.0f, 2.0f },
        std::pair{ 16.0f, 2.0f },
        std::pair{ 30.0f, 5.0f },
        std::pair{ 60.0f, 10.0f },
        std::pair{ 0.6f, 0.1f },
        std::pair{ 3.0f, 0.5f }
    };

    for (const auto& [halfRange, step] : expected)
        EXPECT_NEAR(ui::widgets::HexagonCore::TickStep(halfRange, 6), step, 1e-4f) << "half range " << halfRange;
}

TEST_F(HexagonTickStepTest, ADegenerateRangeFallsBackToUnitSteps)
{
    EXPECT_NEAR(ui::widgets::HexagonCore::TickStep(0.0f, 6), 1.0f, 1e-6f);
    EXPECT_NEAR(ui::widgets::HexagonCore::TickStep(-5.0f, 6), 1.0f, 1e-6f);
    EXPECT_NEAR(ui::widgets::HexagonCore::TickStep(6.0f, 0), 1.0f, 1e-6f);
}

TEST_F(HexagonPeakTest, TheFirstSampleMovesThePeakByTheAttackFraction)
{
    hexagon.SetSample(0.0f, 0.0f, 0.0f, 10.0f, 0.0f);

    EXPECT_NEAR(hexagon.PeakMagnitude(), 3.0f, 1e-4f);
}

TEST_F(HexagonPeakTest, TheFilterRisesFastAndFallsSlowly)
{
    for (auto i = 0; i != 200; ++i)
        hexagon.SetSample(0.0f, 0.0f, 0.0f, 10.0f, 0.0f);

    const auto settled = hexagon.PeakMagnitude();
    EXPECT_NEAR(settled, 10.0f, 1e-2f);

    hexagon.SetSample(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    EXPECT_NEAR(hexagon.PeakMagnitude(), settled * (1.0f - 0.0015f), 1e-3f);
}

TEST_F(HexagonPeakTest, TwoHundredFallingSamplesDecayFarLessThanOneRisingSampleClimbs)
{
    hexagon.SetSample(0.0f, 0.0f, 0.0f, 100.0f, 0.0f);
    const auto afterOneRise = hexagon.PeakMagnitude();

    for (auto i = 0; i != 200; ++i)
        hexagon.SetSample(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    EXPECT_GT(hexagon.PeakMagnitude(), afterOneRise * 0.7f);
}

TEST_F(HexagonPeakTest, ClearingResetsThePeak)
{
    hexagon.SetSample(0.0f, 0.0f, 0.0f, 10.0f, 0.0f);

    hexagon.Clear();

    EXPECT_NEAR(hexagon.PeakMagnitude(), 0.0f, 1e-6f);
}

TEST_F(HexagonPeakTest, AcquiringSamplesDoesNotRequestARepaint)
{
    ui::backend::recording::RecordingPaintedHost host{ hexagon };

    for (auto i = 0; i != 100; ++i)
        hexagon.SetSample(0.0f, 0.0f, 0.0f, static_cast<float>(i), 0.0f);

    EXPECT_EQ(host.InvalidateCount(), 0u);
}

TEST_F(HexagonPeakTest, ChangingTheDcLinkOrClearingDoesRequestARepaint)
{
    ui::backend::recording::RecordingPaintedHost host{ hexagon };

    hexagon.SetDcLinkVolts(48.0f);
    hexagon.Clear();

    EXPECT_EQ(host.InvalidateCount(), 2u);
}

TEST_F(HexagonCoreTest, TheSixVerticesSitOnACircleSixtyDegreesApart)
{
    for (std::size_t k = 0; k != 6; ++k)
    {
        const auto offset = ui::widgets::HexagonCore::VertexOffset(k, 10.0f);
        EXPECT_NEAR(std::hypot(offset.x, offset.y), 10.0f, 1e-4f);
    }

    const auto first = ui::widgets::HexagonCore::VertexOffset(0, 10.0f);
    EXPECT_NEAR(first.x, 10.0f, 1e-4f);
    EXPECT_NEAR(first.y, 0.0f, 1e-4f);
}

TEST_F(HexagonCoreTest, TheSecondVertexIsAboveTheCentreOnScreen)
{
    const auto second = ui::widgets::HexagonCore::VertexOffset(1, 10.0f);

    EXPECT_LT(second.y, 0.0f);
    EXPECT_GT(second.x, 0.0f);
}

TEST_F(HexagonCoreTest, AnAreaTooSmallForTheMarginsDrawsNothing)
{
    hexagon.Paint(canvas, ui::Rect{ 0.0f, 0.0f, 40.0f, 40.0f });

    EXPECT_TRUE(canvas.Commands().empty());
}

TEST_F(HexagonCoreTest, TheHexagonAndItsInscribedCircleAreBothDrawn)
{
    hexagon.Paint(canvas, bounds);

    EXPECT_EQ(CountOf(CommandKind::DrawPolygon), 1u);
    EXPECT_GE(CountOf(CommandKind::DrawEllipse), 7u);
}

TEST_F(HexagonCoreTest, TheFilledMarkersCarryNoOutline)
{
    hexagon.Paint(canvas, bounds);

    auto suppressed = 0;
    for (const auto& command : canvas.Commands())
        if (command.kind == CommandKind::DrawEllipse && command.pen.style == ui::LineStyle::None)
            ++suppressed;

    EXPECT_GE(suppressed, 6);
}

TEST_F(HexagonCoreTest, AResultantVectorAddsItsArrowheadPolygon)
{
    hexagon.Paint(canvas, bounds);
    const auto withoutSample = CountOf(CommandKind::DrawPolygon);

    canvas.Clear();
    hexagon.SetSample(1.0f, -0.5f, -0.5f, 5.0f, 3.0f);
    hexagon.Paint(canvas, bounds);

    EXPECT_EQ(CountOf(CommandKind::DrawPolygon), withoutSample + 1);
}

TEST_F(HexagonCoreTest, AZeroResultantDrawsNoArrowhead)
{
    hexagon.SetSample(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    hexagon.Paint(canvas, bounds);

    EXPECT_EQ(CountOf(CommandKind::DrawPolygon), 1u);
}

TEST_F(HexagonCoreTest, TheReadOutReportsTheSampleAndItsSector)
{
    hexagon.SetSample(0.0f, 0.0f, 0.0f, 5.0f, 0.0f);
    hexagon.Paint(canvas, bounds);

    auto texts = std::vector<std::string>{};
    for (const auto& command : canvas.Commands())
        if (command.kind == CommandKind::DrawText)
            texts.push_back(command.text);

    EXPECT_THAT(texts, ::testing::Contains(::testing::HasSubstr("|V| =")));
    EXPECT_THAT(texts, ::testing::Contains(::testing::HasSubstr("sector: 1")));
}

TEST_F(HexagonCoreTest, TheReadOutReportsNoSectorForAZeroVector)
{
    hexagon.Paint(canvas, bounds);

    auto texts = std::vector<std::string>{};
    for (const auto& command : canvas.Commands())
        if (command.kind == CommandKind::DrawText)
            texts.push_back(command.text);

    EXPECT_THAT(texts, ::testing::Contains(::testing::HasSubstr("sector: -")));
}

TEST_F(HexagonCoreTest, ADeadDcLinkStillProducesAFiniteLayout)
{
    hexagon.SetDcLinkVolts(0.0f);

    hexagon.Paint(canvas, bounds);

    EXPECT_FALSE(canvas.Commands().empty());
}

TEST_F(HexagonCoreTest, TheMinimumSizeIsSquare)
{
    EXPECT_NEAR(hexagon.MinimumSize().width, hexagon.MinimumSize().height, 1e-6f);
    EXPECT_GT(hexagon.MinimumSize().width, 0.0f);
}
