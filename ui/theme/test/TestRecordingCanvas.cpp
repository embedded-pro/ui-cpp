#include "ui/backend/recording/RecordingCanvas.hpp"
#include <array>
#include <gmock/gmock.h>

namespace
{
    using ui::backend::recording::CommandKind;

    class RecordingCanvasTest
        : public ::testing::Test
    {
    protected:
        ui::backend::recording::RecordingCanvas canvas;
    };
}

TEST_F(RecordingCanvasTest, StartsEmpty)
{
    EXPECT_TRUE(canvas.Commands().empty());
}

TEST_F(RecordingCanvasTest, RecordsPrimitivesInOrder)
{
    canvas.DrawLine(ui::Point{ 0.0f, 0.0f }, ui::Point{ 1.0f, 1.0f });
    canvas.DrawRect(ui::Rect{ 0.0f, 0.0f, 2.0f, 2.0f });

    ASSERT_EQ(canvas.Commands().size(), 2u);
    EXPECT_EQ(canvas.Commands()[0].kind, CommandKind::DrawLine);
    EXPECT_EQ(canvas.Commands()[1].kind, CommandKind::DrawRect);
}

TEST_F(RecordingCanvasTest, PolylineRetainsEveryPoint)
{
    const std::array<ui::Point, 3> points{ ui::Point{ 0.0f, 0.0f }, ui::Point{ 1.0f, 2.0f }, ui::Point{ 2.0f, 0.0f } };
    canvas.DrawPolyline(points);

    ASSERT_EQ(canvas.CountOf(CommandKind::DrawPolyline), 1u);
    EXPECT_EQ(canvas.Commands().front().points.size(), 3u);
    EXPECT_NEAR(canvas.Commands().front().points[1].y, 2.0f, 1e-5f);
}

TEST_F(RecordingCanvasTest, PrimitivesCaptureTheActivePen)
{
    const ui::Pen pen{ ui::Color::Rgb(0x2980B9), 2.0f, ui::LineStyle::Dash };
    canvas.SetPen(pen);
    canvas.DrawLine(ui::Point{ 0.0f, 0.0f }, ui::Point{ 1.0f, 0.0f });

    const auto& line = canvas.Commands().back();

    EXPECT_EQ(line.pen.color, pen.color);
    EXPECT_NEAR(line.pen.width, 2.0f, 1e-5f);
    EXPECT_EQ(line.pen.style, ui::LineStyle::Dash);
}

TEST_F(RecordingCanvasTest, TextsReturnsDrawnStringsOnly)
{
    canvas.DrawText(ui::Point{ 0.0f, 0.0f }, "Time (s)");
    canvas.DrawLine(ui::Point{ 0.0f, 0.0f }, ui::Point{ 1.0f, 0.0f });
    canvas.DrawText(ui::Rect{}, ui::TextAlign::Center, ui::TextVerticalAlign::Middle, "0.50");

    EXPECT_THAT(canvas.Texts(), ::testing::ElementsAre("Time (s)", "0.50"));
}

TEST_F(RecordingCanvasTest, MeasureTextScalesWithFontAndLength)
{
    canvas.SetFont(ui::FontSpec{ ui::FontFamily::UiDefault, 10, false, false });
    const auto wide = canvas.MeasureText("wwww");
    const auto narrow = canvas.MeasureText("w");

    EXPECT_GT(wide.width, narrow.width);
    EXPECT_NEAR(wide.height, 10.0f, 1e-5f);
}

TEST_F(RecordingCanvasTest, ClearDiscardsRecordedCommands)
{
    canvas.DrawLine(ui::Point{}, ui::Point{});
    canvas.Clear();

    EXPECT_TRUE(canvas.Commands().empty());
}

TEST_F(RecordingCanvasTest, RecordsTheFullPrimitiveSurface)
{
    const std::array<ui::Point, 3> triangle{ ui::Point{ 0.0f, 0.0f }, ui::Point{ 1.0f, 0.0f }, ui::Point{ 0.5f, 1.0f } };

    canvas.DrawPolygon(triangle);
    canvas.DrawEllipse(ui::Point{ 5.0f, 5.0f }, 2.0f, 3.0f);
    canvas.DrawRoundedRect(ui::Rect{ 0.0f, 0.0f, 10.0f, 4.0f }, 3.0f, 3.0f);
    canvas.FillRect(ui::Rect{ 0.0f, 0.0f, 2.0f, 2.0f }, ui::colors::black);

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawPolygon), 1u);
    EXPECT_EQ(canvas.CountOf(CommandKind::DrawEllipse), 1u);
    EXPECT_EQ(canvas.CountOf(CommandKind::DrawRoundedRect), 1u);
    EXPECT_EQ(canvas.CountOf(CommandKind::FillRect), 1u);
}

TEST_F(RecordingCanvasTest, EllipseRecordsItsCentreAndRadii)
{
    canvas.DrawEllipse(ui::Point{ 5.0f, 6.0f }, 2.0f, 3.0f);
    const auto& command = canvas.Commands().back();

    EXPECT_NEAR(command.from.x, 5.0f, 1e-5f);
    EXPECT_NEAR(command.from.y, 6.0f, 1e-5f);
    EXPECT_NEAR(command.radiusX, 2.0f, 1e-5f);
    EXPECT_NEAR(command.radiusY, 3.0f, 1e-5f);
}

TEST_F(RecordingCanvasTest, FillRectRecordsItsOwnColourNotThePen)
{
    canvas.SetPen(ui::Pen{ ui::Color::Rgb(0x2980B9) });
    canvas.FillRect(ui::Rect{ 0.0f, 0.0f, 1.0f, 1.0f }, ui::Color::Rgb(0xE74C3C));

    EXPECT_EQ(canvas.Commands().back().color, ui::Color::Rgb(0xE74C3C));
}

TEST_F(RecordingCanvasTest, TransformsAndAntialiasingAreRecorded)
{
    canvas.Translate(ui::Point{ 3.0f, 4.0f });
    canvas.Rotate(90.0f);
    canvas.SetAntialiasing(true);

    EXPECT_EQ(canvas.CountOf(CommandKind::Translate), 1u);
    EXPECT_EQ(canvas.CountOf(CommandKind::Rotate), 1u);
    EXPECT_EQ(canvas.CountOf(CommandKind::SetAntialiasing), 1u);
}

TEST_F(RecordingCanvasTest, BrushAndFontAreTrackedAsState)
{
    canvas.SetBrush(ui::Brush{ ui::Color::Rgb(0x27AE60) });
    canvas.SetFont(ui::FontSpec{ ui::FontFamily::Monospace, 11, true, false });
    canvas.DrawRect(ui::Rect{ 0.0f, 0.0f, 1.0f, 1.0f });

    const auto& command = canvas.Commands().back();

    EXPECT_EQ(command.brush.color, ui::Color::Rgb(0x27AE60));
    EXPECT_EQ(canvas.CurrentPen().width, 1.0f);
}

TEST_F(RecordingCanvasTest, ClipCanBeSetAndCleared)
{
    canvas.SetClip(ui::Rect{ 0.0f, 0.0f, 5.0f, 5.0f });
    canvas.ClearClip();

    EXPECT_EQ(canvas.CountOf(CommandKind::SetClip), 1u);
    EXPECT_EQ(canvas.CountOf(CommandKind::ClearClip), 1u);
}

TEST_F(RecordingCanvasTest, StateGuardEmitsSaveAndRestore)
{
    {
        const ui::CanvasStateGuard guard{ canvas };
        canvas.DrawLine(ui::Point{}, ui::Point{});
    }

    EXPECT_EQ(canvas.Commands().front().kind, CommandKind::Save);
    EXPECT_EQ(canvas.Commands().back().kind, CommandKind::Restore);
}
