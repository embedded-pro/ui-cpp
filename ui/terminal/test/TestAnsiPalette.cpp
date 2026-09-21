#include "ui/terminal/AnsiPalette.hpp"
#include <gmock/gmock.h>

namespace
{
    class AnsiPaletteTest
        : public ::testing::Test
    {
    protected:
        ui::terminal::AnsiPalette palette;
    };
}

TEST_F(AnsiPaletteTest, TheDefaultColourResolvesToTheDefaultForegroundOrBackground)
{
    EXPECT_EQ(palette.Foreground(ui::terminal::Color::Default), palette.defaultForeground);
    EXPECT_EQ(palette.Background(ui::terminal::Color::Default), palette.defaultBackground);
}

TEST_F(AnsiPaletteTest, TheEightStandardColoursMapInOrder)
{
    EXPECT_EQ(palette.Foreground(ui::terminal::Color::Black), palette.standard[0]);
    EXPECT_EQ(palette.Foreground(ui::terminal::Color::Red), palette.standard[1]);
    EXPECT_EQ(palette.Foreground(ui::terminal::Color::White), palette.standard[7]);
}

TEST_F(AnsiPaletteTest, TheEightBrightColoursMapInOrder)
{
    EXPECT_EQ(palette.Foreground(ui::terminal::Color::BrightBlack), palette.bright[0]);
    EXPECT_EQ(palette.Foreground(ui::terminal::Color::BrightRed), palette.bright[1]);
    EXPECT_EQ(palette.Foreground(ui::terminal::Color::BrightWhite), palette.bright[7]);
}

TEST_F(AnsiPaletteTest, ForegroundAndBackgroundAgreeOnEveryColourThatIsNotTheDefault)
{
    for (auto value = 1; value <= 16; ++value)
    {
        const auto colour = static_cast<ui::terminal::Color>(value);
        EXPECT_EQ(palette.Foreground(colour), palette.Background(colour)) << "colour " << value;
    }
}
