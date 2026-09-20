#include "ui/backend/qt/QtTheme.hpp"
#include <QPushButton>
#include <gmock/gmock.h>

namespace
{
    class QtThemeTest
        : public ::testing::Test
    {
    protected:
        [[nodiscard]] static QString Colour(ui::theme::ColorRole role, const ui::theme::Theme& theme)
        {
            const auto colour = theme.Get(role);

            return QColor{ colour.red, colour.green, colour.blue, colour.alpha }.name(QColor::HexRgb);
        }
    };

    TEST_F(QtThemeTest, ThePaletteCarriesTheThemesColours)
    {
        const auto palette = ui::backend::qt::ToQtPalette(ui::theme::Light());
        const auto& theme = ui::theme::Light();

        EXPECT_EQ(palette.color(QPalette::Base).name(QColor::HexRgb), Colour(ui::theme::ColorRole::Background, theme));
        EXPECT_EQ(palette.color(QPalette::WindowText).name(QColor::HexRgb), Colour(ui::theme::ColorRole::Text, theme));
        EXPECT_EQ(palette.color(QPalette::Highlight).name(QColor::HexRgb), Colour(ui::theme::ColorRole::Accent, theme));
    }

    TEST_F(QtThemeTest, TheInstrumentPaletteDiffersFromTheLightOne)
    {
        const auto light = ui::backend::qt::ToQtPalette(ui::theme::Light());
        const auto instrument = ui::backend::qt::ToQtPalette(ui::theme::Instrument());

        EXPECT_NE(light.color(QPalette::Base), instrument.color(QPalette::Base));
    }

    TEST_F(QtThemeTest, ApplyingAThemeMakesItCurrentForThePortableLayer)
    {
        ui::backend::qt::ApplyTheme(ui::theme::Instrument());
        EXPECT_EQ(ui::theme::Current().Get(ui::theme::ColorRole::Background),
            ui::theme::Instrument().Get(ui::theme::ColorRole::Background));

        ui::backend::qt::ApplyTheme(ui::theme::Light());
        EXPECT_EQ(ui::theme::Current().Get(ui::theme::ColorRole::Background),
            ui::theme::Light().Get(ui::theme::ColorRole::Background));
    }

    TEST_F(QtThemeTest, EachButtonRoleResolvesToItsSemanticColour)
    {
        const auto& theme = ui::theme::Light();

        EXPECT_THAT(ui::backend::qt::ButtonStyleSheet(ui::theme::ButtonRole::Start, theme).toStdString(),
            ::testing::HasSubstr(Colour(ui::theme::ColorRole::Run, theme).toStdString()));
        EXPECT_THAT(ui::backend::qt::ButtonStyleSheet(ui::theme::ButtonRole::Stop, theme).toStdString(),
            ::testing::HasSubstr(Colour(ui::theme::ColorRole::Stop, theme).toStdString()));
        EXPECT_THAT(ui::backend::qt::ButtonStyleSheet(ui::theme::ButtonRole::EmergencyStop, theme).toStdString(),
            ::testing::HasSubstr(Colour(ui::theme::ColorRole::EmergencyStop, theme).toStdString()));
        EXPECT_THAT(ui::backend::qt::ButtonStyleSheet(ui::theme::ButtonRole::Primary, theme).toStdString(),
            ::testing::HasSubstr(Colour(ui::theme::ColorRole::Accent, theme).toStdString()));
        EXPECT_THAT(ui::backend::qt::ButtonStyleSheet(ui::theme::ButtonRole::Reset, theme).toStdString(),
            ::testing::HasSubstr(Colour(ui::theme::ColorRole::Neutral, theme).toStdString()));
    }

    TEST_F(QtThemeTest, ADefaultButtonKeepsTheReadableTextColour)
    {
        const auto& theme = ui::theme::Light();

        EXPECT_THAT(ui::backend::qt::ButtonStyleSheet(ui::theme::ButtonRole::Default, theme).toStdString(),
            ::testing::HasSubstr("color: " + Colour(ui::theme::ColorRole::Text, theme).toStdString()));
        EXPECT_THAT(ui::backend::qt::ButtonStyleSheet(ui::theme::ButtonRole::Start, theme).toStdString(),
            ::testing::HasSubstr("color: " + Colour(ui::theme::ColorRole::TextInverse, theme).toStdString()));
    }

    TEST_F(QtThemeTest, StylingAButtonInstallsTheRulesForItsRole)
    {
        QPushButton button;
        ui::backend::qt::StyleButton(button, ui::theme::ButtonRole::Stop, ui::theme::Light());

        EXPECT_EQ(button.styleSheet(), ui::backend::qt::ButtonStyleSheet(ui::theme::ButtonRole::Stop, ui::theme::Light()));
    }

    TEST_F(QtThemeTest, StylingAButtonWithoutAThemeUsesTheCurrentOne)
    {
        ui::backend::qt::ApplyTheme(ui::theme::Instrument());

        QPushButton button;
        ui::backend::qt::StyleButton(button, ui::theme::ButtonRole::Primary);

        EXPECT_EQ(button.styleSheet(), ui::backend::qt::ButtonStyleSheet(ui::theme::ButtonRole::Primary, ui::theme::Instrument()));

        ui::backend::qt::ApplyTheme(ui::theme::Light());
    }
}
