#include "ui/backend/qt/QtConversions.hpp"
#include <gmock/gmock.h>

namespace
{
    class QtConversionsTest
        : public ::testing::Test
    {
    };
}

TEST_F(QtConversionsTest, EveryMappedKeyRoundsTripToItsRole)
{
    using ui::backend::qt::ToUiKey;

    EXPECT_EQ(ToUiKey(::Qt::Key_Return), ui::Key::Enter);
    EXPECT_EQ(ToUiKey(::Qt::Key_Enter), ui::Key::Enter);
    EXPECT_EQ(ToUiKey(::Qt::Key_Backspace), ui::Key::Backspace);
    EXPECT_EQ(ToUiKey(::Qt::Key_Tab), ui::Key::Tab);
    EXPECT_EQ(ToUiKey(::Qt::Key_Escape), ui::Key::Escape);
    EXPECT_EQ(ToUiKey(::Qt::Key_Delete), ui::Key::Delete);
    EXPECT_EQ(ToUiKey(::Qt::Key_Home), ui::Key::Home);
    EXPECT_EQ(ToUiKey(::Qt::Key_End), ui::Key::End);
    EXPECT_EQ(ToUiKey(::Qt::Key_PageUp), ui::Key::PageUp);
    EXPECT_EQ(ToUiKey(::Qt::Key_PageDown), ui::Key::PageDown);
    EXPECT_EQ(ToUiKey(::Qt::Key_Up), ui::Key::Up);
    EXPECT_EQ(ToUiKey(::Qt::Key_Down), ui::Key::Down);
    EXPECT_EQ(ToUiKey(::Qt::Key_Left), ui::Key::Left);
    EXPECT_EQ(ToUiKey(::Qt::Key_Right), ui::Key::Right);
}

TEST_F(QtConversionsTest, AnUnmappedKeyIsUnknownRatherThanMisread)
{
    EXPECT_EQ(ui::backend::qt::ToUiKey(::Qt::Key_F1), ui::Key::Unknown);
    EXPECT_EQ(ui::backend::qt::ToUiKey(::Qt::Key_Z), ui::Key::Unknown);
}

TEST_F(QtConversionsTest, EveryMouseButtonMapsToItsRole)
{
    using ui::backend::qt::ToUi;

    EXPECT_EQ(ToUi(::Qt::LeftButton), ui::MouseButton::Left);
    EXPECT_EQ(ToUi(::Qt::RightButton), ui::MouseButton::Right);
    EXPECT_EQ(ToUi(::Qt::MiddleButton), ui::MouseButton::Middle);
    EXPECT_EQ(ToUi(::Qt::NoButton), ui::MouseButton::None);
    EXPECT_EQ(ToUi(::Qt::BackButton), ui::MouseButton::None);
}

TEST_F(QtConversionsTest, LineStylesMapToTheirQtPenStyles)
{
    using ui::backend::qt::ToQt;

    EXPECT_EQ(ToQt(ui::LineStyle::Solid), ::Qt::SolidLine);
    EXPECT_EQ(ToQt(ui::LineStyle::Dash), ::Qt::DashLine);
    EXPECT_EQ(ToQt(ui::LineStyle::Dot), ::Qt::DotLine);
}

TEST_F(QtConversionsTest, APenCarriesItsColourWidthAndStyle)
{
    const auto pen = ui::backend::qt::ToQt(ui::Pen{ ui::Color::Rgb(0x2980B9), 2.5f, ui::LineStyle::Dash });

    EXPECT_EQ(pen.color(), QColor(0x29, 0x80, 0xB9));
    EXPECT_DOUBLE_EQ(pen.widthF(), 2.5);
    EXPECT_EQ(pen.style(), ::Qt::DashLine);
}

TEST_F(QtConversionsTest, ATransparentBrushBecomesNoBrushRatherThanAClearFill)
{
    EXPECT_EQ(ui::backend::qt::ToQt(ui::Brush{ ui::colors::transparent }).style(), ::Qt::NoBrush);
    EXPECT_EQ(ui::backend::qt::ToQt(ui::Brush{ ui::colors::black }).style(), ::Qt::SolidPattern);
}

TEST_F(QtConversionsTest, MonospaceAndDefaultFontsAreDistinguished)
{
    const auto monospace = ui::backend::qt::ToQt(ui::FontSpec{ ui::FontFamily::Monospace, 11, true, true });
    const auto standard = ui::backend::qt::ToQt(ui::FontSpec{ ui::FontFamily::UiDefault, 9, false, false });

    EXPECT_EQ(monospace.styleHint(), QFont::Monospace);
    EXPECT_EQ(monospace.pointSize(), 11);
    EXPECT_TRUE(monospace.bold());
    EXPECT_TRUE(monospace.italic());

    EXPECT_EQ(standard.styleHint(), QFont::SansSerif);
    EXPECT_FALSE(standard.bold());
}

TEST_F(QtConversionsTest, AlignmentCombinesTheHorizontalAndVerticalRoles)
{
    using ui::backend::qt::ToQt;

    EXPECT_EQ(ToQt(ui::TextAlign::Left, ui::TextVerticalAlign::Top), ::Qt::AlignLeft | ::Qt::AlignTop);
    EXPECT_EQ(ToQt(ui::TextAlign::Center, ui::TextVerticalAlign::Middle), ::Qt::AlignHCenter | ::Qt::AlignVCenter);
    EXPECT_EQ(ToQt(ui::TextAlign::Right, ui::TextVerticalAlign::Bottom), ::Qt::AlignRight | ::Qt::AlignBottom);

    // Qt aligns by box, not baseline, so Baseline is centred rather than silently dropped.
    EXPECT_EQ(ToQt(ui::TextAlign::Left, ui::TextVerticalAlign::Baseline), ::Qt::AlignLeft | ::Qt::AlignVCenter);
}

TEST_F(QtConversionsTest, ModifiersAreCarriedIndividually)
{
    const auto all = ui::backend::qt::ToUi(::Qt::ShiftModifier | ::Qt::ControlModifier | ::Qt::AltModifier);
    const auto none = ui::backend::qt::ToUi(::Qt::KeyboardModifiers{ ::Qt::NoModifier });

    EXPECT_TRUE(all.shift);
    EXPECT_TRUE(all.control);
    EXPECT_TRUE(all.alt);

    EXPECT_FALSE(none.shift);
    EXPECT_FALSE(none.control);
    EXPECT_FALSE(none.alt);
}

TEST_F(QtConversionsTest, GeometryCrossesBothWaysUnchanged)
{
    const ui::Rect source{ 1.5f, 2.5f, 10.0f, 20.0f };
    const auto roundTripped = ui::backend::qt::ToUi(ui::backend::qt::ToQt(source));

    EXPECT_NEAR(roundTripped.x, source.x, 1e-3f);
    EXPECT_NEAR(roundTripped.y, source.y, 1e-3f);
    EXPECT_NEAR(roundTripped.width, source.width, 1e-3f);
    EXPECT_NEAR(roundTripped.height, source.height, 1e-3f);

    const auto point = ui::backend::qt::ToUi(ui::backend::qt::ToQt(ui::Point{ 3.25f, 4.75f }));
    EXPECT_NEAR(point.x, 3.25f, 1e-3f);
    EXPECT_NEAR(point.y, 4.75f, 1e-3f);
}

TEST_F(QtConversionsTest, ColourCarriesItsAlpha)
{
    const auto colour = ui::backend::qt::ToQt(ui::Color{ 10, 20, 30, 40 });

    EXPECT_EQ(colour.red(), 10);
    EXPECT_EQ(colour.green(), 20);
    EXPECT_EQ(colour.blue(), 30);
    EXPECT_EQ(colour.alpha(), 40);
}

TEST_F(QtConversionsTest, ANonTerminatedViewConvertsByLength)
{
    const std::string_view source{ "abcdef" };

    EXPECT_EQ(ui::backend::qt::ToQt(source.substr(0, 3)), QStringLiteral("abc"));
    EXPECT_EQ(ui::backend::qt::ToQt(std::string_view{}), QString{});
}
