#include "ui/terminal/TerminalTypes.hpp"
#include <gmock/gmock.h>

namespace
{
    class TerminalTypesTest
        : public ::testing::Test
    {
    };
}

TEST_F(TerminalTypesTest, ARenditionDefaultsToPlainDefaultColours)
{
    const ui::terminal::Rendition rendition;

    EXPECT_EQ(rendition.foreground, ui::terminal::Color::Default);
    EXPECT_EQ(rendition.background, ui::terminal::Color::Default);
    EXPECT_FALSE(rendition.bold);
    EXPECT_FALSE(rendition.faint);
    EXPECT_FALSE(rendition.italic);
    EXPECT_FALSE(rendition.underline);
    EXPECT_FALSE(rendition.blink);
    EXPECT_FALSE(rendition.inverse);
    EXPECT_EQ(rendition, ui::terminal::Rendition{});
}

TEST_F(TerminalTypesTest, ACellDefaultsToBlankWithTheDefaultRendition)
{
    const ui::terminal::Cell cell;

    EXPECT_EQ(cell.codepoint, U' ');
    EXPECT_EQ(cell.rendition, ui::terminal::Rendition{});
    EXPECT_EQ(cell, ui::terminal::Cell{});
}

TEST_F(TerminalTypesTest, ACursorPositionDefaultsToTheOrigin)
{
    const ui::terminal::CursorPosition cursor;

    EXPECT_EQ(cursor.row, 0);
    EXPECT_EQ(cursor.column, 0);
    EXPECT_EQ(cursor, ui::terminal::CursorPosition{});
}

TEST_F(TerminalTypesTest, ModesDefaultToTheVt100PowerOnState)
{
    const ui::terminal::Modes modes;

    EXPECT_TRUE(modes.autoWrap);
    EXPECT_FALSE(modes.originMode);
    EXPECT_FALSE(modes.lineFeedNewLine);
    EXPECT_TRUE(modes.cursorVisible);
    EXPECT_FALSE(modes.applicationCursorKeys);
    EXPECT_FALSE(modes.applicationKeypad);
}
