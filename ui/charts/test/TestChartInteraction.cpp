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
    EXPECT_NEAR(interaction.ViewMinimum(), 0.0f, tolerance);
    EXPECT_NEAR(interaction.ViewMaximum(), 10.0f, tolerance);
    EXPECT_FALSE(interaction.IsZoomed());
}

TEST_F(ChartInteractionTest, ZoomingInNarrowsTheView)
{
    interaction.Zoom(scrollUp, 0.5f);

    EXPECT_LT(interaction.ViewMaximum() - interaction.ViewMinimum(), 10.0f);
    EXPECT_TRUE(interaction.IsZoomed());
}

TEST_F(ChartInteractionTest, ZoomKeepsTheCursorAnchored)
{
    interaction.Zoom(scrollUp, 0.5f);

    const auto centre = (interaction.ViewMinimum() + interaction.ViewMaximum()) * 0.5f;
    EXPECT_NEAR(centre, 5.0f, tolerance);
}

TEST_F(ChartInteractionTest, ZoomingOutIsClampedToTheDataRange)
{
    interaction.Zoom(scrollDown, 0.5f);
    interaction.Zoom(scrollDown, 0.5f);
    interaction.Zoom(scrollDown, 0.5f);

    EXPECT_NEAR(interaction.ViewMinimum(), 0.0f, tolerance);
    EXPECT_NEAR(interaction.ViewMaximum(), 10.0f, tolerance);
}

TEST_F(ChartInteractionTest, ZoomOnADegenerateRangeIsIgnored)
{
    interaction.SetDataRange(1.0f, 1.0f);
    interaction.Zoom(scrollUp, 0.5f);

    EXPECT_NEAR(interaction.ViewMinimum(), 1.0f, tolerance);
    EXPECT_NEAR(interaction.ViewMaximum(), 1.0f, tolerance);
}

TEST_F(ChartInteractionTest, ResetViewRestoresTheFullRange)
{
    interaction.Zoom(scrollUp, 0.25f);
    interaction.ResetView();

    EXPECT_NEAR(interaction.ViewMinimum(), 0.0f, tolerance);
    EXPECT_NEAR(interaction.ViewMaximum(), 10.0f, tolerance);
    EXPECT_FALSE(interaction.IsZoomed());
}

TEST_F(ChartInteractionTest, PanningShiftsTheViewOppositeTheDrag)
{
    interaction.Zoom(scrollUp, 0.5f);
    const auto before = interaction.ViewMinimum();

    interaction.StartPan(ui::Point{ 100.0f, 0.0f });
    interaction.UpdatePan(ui::Point{ 50.0f, 0.0f }, 200.0f);

    EXPECT_GT(interaction.ViewMinimum(), before);
}

TEST_F(ChartInteractionTest, PanningPreservesTheViewSpan)
{
    interaction.Zoom(scrollUp, 0.5f);
    const auto span = interaction.ViewMaximum() - interaction.ViewMinimum();

    interaction.StartPan(ui::Point{ 100.0f, 0.0f });
    interaction.UpdatePan(ui::Point{ 60.0f, 0.0f }, 200.0f);

    EXPECT_NEAR(interaction.ViewMaximum() - interaction.ViewMinimum(), span, tolerance);
}

TEST_F(ChartInteractionTest, PanningIsClampedToTheDataRange)
{
    interaction.Zoom(scrollUp, 0.5f);

    interaction.StartPan(ui::Point{ 1000.0f, 0.0f });
    interaction.UpdatePan(ui::Point{ 0.0f, 0.0f }, 200.0f);

    EXPECT_LE(interaction.ViewMaximum(), 10.0f + tolerance);
    EXPECT_GE(interaction.ViewMinimum(), -tolerance);
}

TEST_F(ChartInteractionTest, UpdatePanWithoutStartPanIsIgnored)
{
    const auto before = interaction.ViewMinimum();
    interaction.UpdatePan(ui::Point{ 50.0f, 0.0f }, 200.0f);

    EXPECT_NEAR(interaction.ViewMinimum(), before, tolerance);
}

TEST_F(ChartInteractionTest, PanningStateTracksStartAndEnd)
{
    EXPECT_FALSE(interaction.IsPanning());

    interaction.StartPan(ui::Point{ 0.0f, 0.0f });
    EXPECT_TRUE(interaction.IsPanning());

    interaction.EndPan();
    EXPECT_FALSE(interaction.IsPanning());
}

TEST_F(ChartInteractionTest, ViewSpanTracksTheView)
{
    EXPECT_NEAR(interaction.ViewSpan(), 10.0f, tolerance);

    interaction.Zoom(scrollUp, 0.5f);

    EXPECT_LT(interaction.ViewSpan(), 10.0f);
    EXPECT_NEAR(interaction.ViewSpan(), interaction.ViewMaximum() - interaction.ViewMinimum(), tolerance);
}

TEST_F(ChartInteractionTest, CrosshairIsHiddenUntilShown)
{
    EXPECT_FALSE(interaction.CrosshairVisible());
}

TEST_F(ChartInteractionTest, ShowCrosshairAtRecordsTheCursor)
{
    interaction.ShowCrosshairAt(ui::Point{ 12.0f, 34.0f });

    EXPECT_TRUE(interaction.CrosshairVisible());
    EXPECT_NEAR(interaction.CursorPosition().x, 12.0f, tolerance);
    EXPECT_NEAR(interaction.CursorPosition().y, 34.0f, tolerance);
}

TEST_F(ChartInteractionTest, HideCrosshairKeepsTheLastCursorPosition)
{
    interaction.ShowCrosshairAt(ui::Point{ 12.0f, 34.0f });
    interaction.HideCrosshair();

    EXPECT_FALSE(interaction.CrosshairVisible());
    EXPECT_NEAR(interaction.CursorPosition().x, 12.0f, tolerance);
}

TEST_F(ChartInteractionTest, ZeroWidthPanIsIgnored)
{
    interaction.Zoom(scrollUp, 0.5f);
    const auto before = interaction.ViewMinimum();

    interaction.StartPan(ui::Point{ 100.0f, 0.0f });
    interaction.UpdatePan(ui::Point{ 50.0f, 0.0f }, 0.0f);

    EXPECT_NEAR(interaction.ViewMinimum(), before, tolerance);
}
