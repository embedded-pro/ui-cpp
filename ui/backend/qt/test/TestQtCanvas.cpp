#include "ui/backend/qt/QtCanvas.hpp"
#include "ui/backend/qt/test/QtTestSupport.hpp"
#include <QPainter>
#include <gmock/gmock.h>

namespace
{
    using ui::backend::qt::test::HasInk;
    using ui::backend::qt::test::InkCount;

    constexpr QRgb background{ 0xFFFFFFFFu };

    class QtCanvasTest
        : public ::testing::Test
    {
    protected:
        QtCanvasTest()
        {
            image.fill(background);
            painter.begin(&image);
            canvas.Bind(painter);
            canvas.SetPen(ui::Pen{ ui::colors::black });
        }

        ~QtCanvasTest() override
        {
            if (painter.isActive())
                painter.end();
        }

        void Finish()
        {
            painter.end();
        }

        QImage image{ 200, 160, QImage::Format_ARGB32 };
        QPainter painter;
        ui::backend::qt::QtCanvas canvas;
    };
}

TEST_F(QtCanvasTest, DrawLineMarksTheSegment)
{
    canvas.DrawLine(ui::Point{ 10.0f, 80.0f }, ui::Point{ 190.0f, 80.0f });
    Finish();

    EXPECT_TRUE(HasInk(image, QRect{ 10, 78, 180, 5 }, background));
    EXPECT_FALSE(HasInk(image, QRect{ 0, 0, 200, 60 }, background));
}

TEST_F(QtCanvasTest, DrawPolylineMarksEverySegment)
{
    const std::array<ui::Point, 3> points{ ui::Point{ 10.0f, 10.0f }, ui::Point{ 100.0f, 150.0f }, ui::Point{ 190.0f, 10.0f } };
    canvas.DrawPolyline(points);
    Finish();

    EXPECT_TRUE(HasInk(image, QRect{ 40, 40, 20, 40 }, background));
    EXPECT_TRUE(HasInk(image, QRect{ 140, 40, 20, 40 }, background));
}

TEST_F(QtCanvasTest, DrawPolygonFillsWithTheCurrentBrush)
{
    canvas.SetBrush(ui::Brush{ ui::Color::Rgb(0x2980B9) });

    const std::array<ui::Point, 3> points{ ui::Point{ 20.0f, 20.0f }, ui::Point{ 180.0f, 20.0f }, ui::Point{ 100.0f, 140.0f } };
    canvas.DrawPolygon(points);
    Finish();

    EXPECT_EQ(image.pixel(100, 60), qRgb(0x29, 0x80, 0xB9));
}

TEST_F(QtCanvasTest, TransparentBrushLeavesTheInteriorUntouched)
{
    canvas.SetBrush(ui::Brush{ ui::colors::transparent });
    canvas.DrawRect(ui::Rect{ 20.0f, 20.0f, 160.0f, 120.0f });
    Finish();

    EXPECT_EQ(image.pixel(100, 80), background);
    EXPECT_TRUE(HasInk(image, QRect{ 18, 18, 5, 5 }, background));
}

TEST_F(QtCanvasTest, FillRectPaintsTheGivenColour)
{
    canvas.FillRect(ui::Rect{ 0.0f, 0.0f, 100.0f, 80.0f }, ui::Color::Rgb(0xE74C3C));
    Finish();

    EXPECT_EQ(image.pixel(50, 40), qRgb(0xE7, 0x4C, 0x3C));
    EXPECT_EQ(image.pixel(150, 120), background);
}

TEST_F(QtCanvasTest, DrawRoundedRectLeavesTheCornersClear)
{
    canvas.DrawRoundedRect(ui::Rect{ 10.0f, 10.0f, 180.0f, 140.0f }, 20.0f, 20.0f);
    Finish();

    EXPECT_EQ(image.pixel(10, 10), background);
    EXPECT_TRUE(HasInk(image, QRect{ 90, 8, 20, 5 }, background));
}

TEST_F(QtCanvasTest, DrawEllipseIsCentredOnThePoint)
{
    canvas.DrawEllipse(ui::Point{ 100.0f, 80.0f }, 40.0f, 20.0f);
    Finish();

    EXPECT_TRUE(HasInk(image, QRect{ 138, 78, 5, 5 }, background));
    EXPECT_FALSE(HasInk(image, QRect{ 0, 0, 40, 40 }, background));
}

TEST_F(QtCanvasTest, ClipSuppressesDrawingOutsideTheRegion)
{
    canvas.SetClip(ui::Rect{ 0.0f, 0.0f, 100.0f, 160.0f });
    canvas.DrawLine(ui::Point{ 0.0f, 80.0f }, ui::Point{ 200.0f, 80.0f });
    Finish();

    EXPECT_TRUE(HasInk(image, QRect{ 0, 78, 100, 5 }, background));
    EXPECT_FALSE(HasInk(image, QRect{ 101, 0, 99, 160 }, background));
}

TEST_F(QtCanvasTest, ClearClipRestoresTheFullSurface)
{
    canvas.SetClip(ui::Rect{ 0.0f, 0.0f, 100.0f, 160.0f });
    canvas.ClearClip();
    canvas.DrawLine(ui::Point{ 0.0f, 80.0f }, ui::Point{ 200.0f, 80.0f });
    Finish();

    EXPECT_TRUE(HasInk(image, QRect{ 101, 78, 99, 5 }, background));
}

TEST_F(QtCanvasTest, RestoreUndoesThePenSetAfterSave)
{
    canvas.SetPen(ui::Pen{ ui::Color::Rgb(0xE74C3C) });
    canvas.Save();
    canvas.SetPen(ui::Pen{ ui::Color::Rgb(0x27AE60) });
    canvas.Restore();
    canvas.DrawLine(ui::Point{ 0.0f, 80.0f }, ui::Point{ 200.0f, 80.0f });
    Finish();

    EXPECT_EQ(image.pixel(100, 80), qRgb(0xE7, 0x4C, 0x3C));
}

TEST_F(QtCanvasTest, TranslateOffsetsSubsequentDrawing)
{
    canvas.Translate(ui::Point{ 0.0f, 40.0f });
    canvas.DrawLine(ui::Point{ 0.0f, 40.0f }, ui::Point{ 200.0f, 40.0f });
    Finish();

    EXPECT_TRUE(HasInk(image, QRect{ 0, 78, 200, 5 }, background));
    EXPECT_FALSE(HasInk(image, QRect{ 0, 0, 200, 60 }, background));
}

TEST_F(QtCanvasTest, RotateTurnsTheDrawingFrame)
{
    canvas.Translate(ui::Point{ 100.0f, 80.0f });
    canvas.Rotate(90.0f);
    canvas.DrawLine(ui::Point{ -60.0f, 0.0f }, ui::Point{ 60.0f, 0.0f });
    Finish();

    EXPECT_TRUE(HasInk(image, QRect{ 98, 20, 5, 120 }, background));
    EXPECT_FALSE(HasInk(image, QRect{ 0, 0, 90, 160 }, background));
}

TEST_F(QtCanvasTest, DrawTextPutsInkAboveTheBaseline)
{
    canvas.SetFont(ui::FontSpec{ ui::FontFamily::UiDefault, 20, false, false });
    canvas.DrawText(ui::Point{ 20.0f, 100.0f }, "Hg");
    Finish();

    EXPECT_TRUE(HasInk(image, QRect{ 20, 70, 60, 30 }, background));
}

TEST_F(QtCanvasTest, DrawTextCentresWithinTheRectangle)
{
    canvas.SetFont(ui::FontSpec{ ui::FontFamily::UiDefault, 20, false, false });
    canvas.DrawText(ui::Rect{ 0.0f, 0.0f, 200.0f, 160.0f }, ui::TextAlign::Center, ui::TextVerticalAlign::Middle, "Hg");
    Finish();

    EXPECT_GT(InkCount(image, QRect{ 60, 50, 80, 60 }, background), InkCount(image, QRect{ 0, 0, 40, 40 }, background));
}

TEST_F(QtCanvasTest, DrawTextHandlesANonTerminatedView)
{
    canvas.SetFont(ui::FontSpec{ ui::FontFamily::UiDefault, 20, false, false });

    const std::string_view source{ "Hg noise" };
    canvas.DrawText(ui::Point{ 20.0f, 100.0f }, source.substr(0, 2));
    Finish();

    EXPECT_FALSE(HasInk(image, QRect{ 90, 60, 110, 50 }, background));
}

TEST_F(QtCanvasTest, MeasureTextGrowsWithTheTextAndTheFont)
{
    canvas.SetFont(ui::FontSpec{ ui::FontFamily::UiDefault, 10, false, false });
    const auto shortText = canvas.MeasureText("1.0");
    const auto longText = canvas.MeasureText("1.000000");

    canvas.SetFont(ui::FontSpec{ ui::FontFamily::UiDefault, 20, false, false });
    const auto larger = canvas.MeasureText("1.0");

    EXPECT_GT(shortText.width, 0.0f);
    EXPECT_GT(shortText.height, 0.0f);
    EXPECT_GT(longText.width, shortText.width);
    EXPECT_GT(larger.width, shortText.width);
}

TEST_F(QtCanvasTest, MeasureTextIsEmptyForEmptyText)
{
    EXPECT_NEAR(canvas.MeasureText("").width, 0.0f, 1e-3f);
}
