#include "ui/backend/recording/RecordingCanvas.hpp"
#include "ui/scene/SceneGizmos.hpp"
#include "ui/theme/Theme.hpp"
#include <cmath>
#include <gmock/gmock.h>
#include <numbers>

namespace
{
    using ui::backend::recording::CommandKind;
    using ui::scene::CameraPose;
    using ui::scene::Vector3;
    using ui::scene::ViewFrame;

    class SceneGizmosTest
        : public ::testing::Test
    {
    protected:
        SceneGizmosTest()
        {
            ui::theme::SetCurrent(ui::theme::Instrument());
        }

        [[nodiscard]] static bool AllFinite(const ui::backend::recording::RecordingCanvas& canvas)
        {
            for (const auto& command : canvas.Commands())
                if (!std::isfinite(command.from.x) || !std::isfinite(command.from.y) || !std::isfinite(command.to.x) || !std::isfinite(command.to.y))
                    return false;

            return true;
        }

        ui::backend::recording::RecordingCanvas canvas;
        ViewFrame frame{ CameraPose{}, ui::Rect{ 0.0f, 0.0f, 800.0f, 600.0f }, {} };
    };
}

TEST_F(SceneGizmosTest, TheDefaultGridHasOneLinePerDivisionBoundaryInBothDirections)
{
    ui::scene::DrawGroundGrid(canvas, frame);

    // extent 2.0 at step 0.25 is 17 lines each way, matching the Qt original's float loop.
    EXPECT_EQ(canvas.CountOf(CommandKind::DrawLine), 34u);
}

TEST_F(SceneGizmosTest, TheGridLineCountFollowsTheConfiguredStep)
{
    ui::scene::DrawGroundGrid(canvas, frame, ui::scene::GroundGridConfig{ 1.0f, 0.5f, 1.0f });

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawLine), 10u);
}

TEST_F(SceneGizmosTest, TheGridUsesTheGridMajorRole)
{
    ui::scene::DrawGroundGrid(canvas, frame);

    EXPECT_EQ(canvas.CurrentPen().color, ui::theme::Instrument().Get(ui::theme::ColorRole::GridMajor));
    EXPECT_NEAR(canvas.CurrentPen().width, 1.0f, 1e-6f);
}

TEST_F(SceneGizmosTest, AZeroStepDrawsNothingRatherThanLoopingForever)
{
    ui::scene::DrawGroundGrid(canvas, frame, ui::scene::GroundGridConfig{ 2.0f, 0.0f, 1.0f });

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawLine), 0u);
}

TEST_F(SceneGizmosTest, TheTriadIsThreeLinesAndThreeLabels)
{
    ui::scene::DrawAxisTriad(canvas, frame);

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawLine), 3u);
    EXPECT_EQ(canvas.CountOf(CommandKind::DrawText), 3u);
    EXPECT_THAT(canvas.Texts(), ::testing::ElementsAre("X", "Y", "Z"));
}

TEST_F(SceneGizmosTest, EveryTriadLineStartsAtTheProjectedOrigin)
{
    ui::scene::DrawAxisTriad(canvas, frame);

    const auto origin = frame.Project(Vector3{});

    for (const auto& command : canvas.Commands())
        if (command.kind == CommandKind::DrawLine)
        {
            EXPECT_NEAR(command.from.x, origin.x, 1e-3f);
            EXPECT_NEAR(command.from.y, origin.y, 1e-3f);
        }
}

// Not the Series palette: series colours are for data, and retinting them must not silently
// recolour the world axes.
TEST_F(SceneGizmosTest, EachAxisCarriesItsOwnRole)
{
    ui::scene::DrawAxisTriad(canvas, frame);

    const auto& theme = ui::theme::Instrument();
    std::vector<ui::Color> lineColors;

    for (const auto& command : canvas.Commands())
        if (command.kind == CommandKind::SetPen)
            lineColors.push_back(command.pen.color);

    ASSERT_GE(lineColors.size(), 3u);
    EXPECT_EQ(lineColors[0], theme.Get(ui::theme::ColorRole::SceneAxisX));
    EXPECT_EQ(lineColors[1], theme.Get(ui::theme::ColorRole::SceneAxisY));
    EXPECT_EQ(lineColors[2], theme.Get(ui::theme::ColorRole::SceneAxisZ));
}

TEST_F(SceneGizmosTest, TheTriadLabelsUseTheMonospaceRole)
{
    ui::scene::DrawAxisTriad(canvas, frame);

    for (const auto& command : canvas.Commands())
        if (command.kind == CommandKind::SetFont)
        {
            EXPECT_EQ(command.font, ui::theme::Instrument().Get(ui::theme::FontRole::Monospace));
        }
}

TEST_F(SceneGizmosTest, ADegenerateCameraStillProducesFiniteGeometry)
{
    ViewFrame overhead{ CameraPose{ 0.0f, std::numbers::pi_v<float> / 2.0f, 3.0f, Vector3{} },
        ui::Rect{ 0.0f, 0.0f, 800.0f, 600.0f }, {} };

    ui::scene::DrawGroundGrid(canvas, overhead);
    ui::scene::DrawAxisTriad(canvas, overhead);

    EXPECT_TRUE(AllFinite(canvas));
}

TEST_F(SceneGizmosTest, TheGizmosDoNotInheritAFillFromEarlierDrawing)
{
    canvas.SetBrush(ui::Brush{ ui::colors::white });
    ui::scene::DrawGroundGrid(canvas, frame);

    const auto& commands = canvas.Commands();
    const auto brushReset = std::find_if(commands.begin(), commands.end(),
        [](const auto& command)
        {
            return command.kind == CommandKind::SetBrush && command.brush.color.alpha == 0;
        });

    EXPECT_NE(brushReset, commands.end());
}
