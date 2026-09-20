#include "ui/backend/recording/RecordingCanvas.hpp"
#include "ui/theme/Theme.hpp"
#include <gmock/gmock.h>

namespace
{
    class ThemeTest
        : public ::testing::Test
    {
    protected:
        ui::backend::recording::RecordingCanvas canvas;
    };
}

TEST_F(ThemeTest, SeriesColorsWrapAroundThePalette)
{
    const auto& theme = ui::theme::Light();

    EXPECT_EQ(theme.Series(0), theme.Get(ui::theme::ColorRole::Series0));
    EXPECT_EQ(theme.Series(7), theme.Get(ui::theme::ColorRole::Series7));
    EXPECT_EQ(theme.Series(8), theme.Series(0));
}

TEST_F(ThemeTest, LightAndInstrumentDifferInChrome)
{
    EXPECT_NE(ui::theme::Light().Get(ui::theme::ColorRole::Background),
        ui::theme::Instrument().Get(ui::theme::ColorRole::Background));
}

TEST_F(ThemeTest, BothThemesShareScopeBackground)
{
    EXPECT_EQ(ui::theme::Light().Get(ui::theme::ColorRole::ScopeBackground),
        ui::theme::Instrument().Get(ui::theme::ColorRole::ScopeBackground));
}

TEST_F(ThemeTest, CurrentDefaultsToLight)
{
    EXPECT_EQ(ui::theme::Current().Get(ui::theme::ColorRole::Background),
        ui::theme::Light().Get(ui::theme::ColorRole::Background));
}

TEST_F(ThemeTest, SetCurrentSwitchesTheActiveTheme)
{
    ui::theme::SetCurrent(ui::theme::Instrument());

    EXPECT_EQ(ui::theme::Current().Get(ui::theme::ColorRole::Background),
        ui::theme::Instrument().Get(ui::theme::ColorRole::Background));

    ui::theme::SetCurrent(ui::theme::Light());
}

TEST_F(ThemeTest, ChartMetricsCarryTheInheritedMargins)
{
    const auto& metrics = ui::theme::Light().Charts();

    EXPECT_EQ(metrics.leftMargin, 65);
    EXPECT_EQ(metrics.rightMargin, 20);
    EXPECT_EQ(metrics.topMargin, 15);
    EXPECT_EQ(metrics.bottomMargin, 35);
    EXPECT_EQ(metrics.panelSpacing, 45);
    EXPECT_EQ(metrics.gridLines, 5);
}
