#include "ui/charts/ChartInteraction.hpp"
#include <gmock/gmock.h>

namespace
{
    class ChartInteractionTest
        : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            interaction.SetDataRange(0.0f, 10.0f);
        }

        ui::charts::ChartInteraction interaction;

        static constexpr float tolerance{ 1e-4f };
        static constexpr float scrollUp{ 1.0f };
        static constexpr float scrollDown{ -1.0f };
    };
}

TEST_F(ChartInteractionTest, SetDataRangeResetsTheView)
{
    EXPECT_NEAR(interaction.viewMinimum, 0.0f, tolerance);
    EXPECT_NEAR(interaction.viewMaximum, 10.0f, tolerance);
    EXPECT_FALSE(interaction.IsZoomed());
}

TEST_F(ChartInteractionTest, ZoomingInNarrowsTheView)
{
    interaction.Zoom(scrollUp, 0.5f);

    EXPECT_LT(interaction.viewMaximum - interaction.viewMinimum, 10.0f);
    EXPECT_TRUE(interaction.IsZoomed());
}

TEST_F(ChartInteractionTest, ZoomKeepsTheCursorAnchored)
{
    interaction.Zoom(scrollUp, 0.5f);

    const auto centre = (interaction.viewMinimum + interaction.viewMaximum) * 0.5f;
    EXPECT_NEAR(centre, 5.0f, tolerance);
}

TEST_F(ChartInteractionTest, ZoomingOutIsClampedToTheDataRange)
{
    interaction.Zoom(scrollDown, 0.5f);
    interaction.Zoom(scrollDown, 0.5f);
    interaction.Zoom(scrollDown, 0.5f);

    EXPECT_NEAR(interaction.viewMinimum, 0.0f, tolerance);
    EXPECT_NEAR(interaction.viewMaximum, 10.0f, tolerance);
}

TEST_F(ChartInteractionTest, ZoomOnADegenerateRangeIsIgnored)
{
    interaction.SetDataRange(1.0f, 1.0f);
    interaction.Zoom(scrollUp, 0.5f);

    EXPECT_NEAR(interaction.viewMinimum, 1.0f, tolerance);
    EXPECT_NEAR(interaction.viewMaximum, 1.0f, tolerance);
}

TEST_F(ChartInteractionTest, ResetViewRestoresTheFullRange)
{
    interaction.Zoom(scrollUp, 0.25f);
    interaction.ResetView();

    EXPECT_NEAR(interaction.viewMinimum, 0.0f, tolerance);
    EXPECT_NEAR(interaction.viewMaximum, 10.0f, tolerance);
    EXPECT_FALSE(interaction.IsZoomed());
}

TEST_F(ChartInteractionTest, PanningShiftsTheViewOppositeTheDrag)
{
    interaction.Zoom(scrollUp, 0.5f);
    const auto before = interaction.viewMinimum;

    interaction.StartPan(ui::Point{ 100.0f, 0.0f });
    interaction.UpdatePan(ui::Point{ 50.0f, 0.0f }, 200.0f);

    EXPECT_GT(interaction.viewMinimum, before);
}

TEST_F(ChartInteractionTest, PanningPreservesTheViewSpan)
{
    interaction.Zoom(scrollUp, 0.5f);
    const auto span = interaction.viewMaximum - interaction.viewMinimum;

    interaction.StartPan(ui::Point{ 100.0f, 0.0f });
    interaction.UpdatePan(ui::Point{ 60.0f, 0.0f }, 200.0f);

    EXPECT_NEAR(interaction.viewMaximum - interaction.viewMinimum, span, tolerance);
}

TEST_F(ChartInteractionTest, PanningIsClampedToTheDataRange)
{
    interaction.Zoom(scrollUp, 0.5f);

    interaction.StartPan(ui::Point{ 1000.0f, 0.0f });
    interaction.UpdatePan(ui::Point{ 0.0f, 0.0f }, 200.0f);

    EXPECT_LE(interaction.viewMaximum, 10.0f + tolerance);
    EXPECT_GE(interaction.viewMinimum, -tolerance);
}

TEST_F(ChartInteractionTest, UpdatePanWithoutStartPanIsIgnored)
{
    const auto before = interaction.viewMinimum;
    interaction.UpdatePan(ui::Point{ 50.0f, 0.0f }, 200.0f);

    EXPECT_NEAR(interaction.viewMinimum, before, tolerance);
}

TEST_F(ChartInteractionTest, PanningStateTracksStartAndEnd)
{
    EXPECT_FALSE(interaction.IsPanning());

    interaction.StartPan(ui::Point{ 0.0f, 0.0f });
    EXPECT_TRUE(interaction.IsPanning());

    interaction.EndPan();
    EXPECT_FALSE(interaction.IsPanning());
}

TEST_F(ChartInteractionTest, ZeroWidthPanIsIgnored)
{
    interaction.Zoom(scrollUp, 0.5f);
    const auto before = interaction.viewMinimum;

    interaction.StartPan(ui::Point{ 100.0f, 0.0f });
    interaction.UpdatePan(ui::Point{ 50.0f, 0.0f }, 0.0f);

    EXPECT_NEAR(interaction.viewMinimum, before, tolerance);
}
