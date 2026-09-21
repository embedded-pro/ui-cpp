#include "ui/backend/qt/QtAppShell.hpp"
#include <QLabel>
#include <QMainWindow>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <array>
#include <gmock/gmock.h>

namespace
{
    using ui::backend::qt::QtAppShell;
    using ui::shell::PageSpec;
    using ui::shell::ShellSpec;

    constexpr std::array<PageSpec, 3> pages{
        PageSpec{ "Time Domain" },
        PageSpec{ "Frequency Response" },
        PageSpec{ "Impulse Response" }
    };

    class QtAppShellTest
        : public ::testing::Test
    {
    protected:
        QMainWindow window;
        ShellSpec spec{ "IIR Filter Simulator", ui::Size{ 1200.0f, 700.0f }, 350.0f, pages, "Configure filter parameters and press Compute" };
        QtAppShell shell{ window, spec };
    };

    class PagelessQtAppShellTest
        : public ::testing::Test
    {
    protected:
        QMainWindow window;
        ShellSpec spec{ "LQG Controller Simulator", ui::Size{ 1280.0f, 800.0f }, 350.0f, {}, "Configure LQG parameters and press Compute" };
        QtAppShell shell{ window, spec };
    };
}

TEST_F(QtAppShellTest, TheWindowTakesItsTitleAndSizeFromTheSpec)
{
    EXPECT_EQ(window.windowTitle(), "IIR Filter Simulator");
    EXPECT_EQ(window.size().width(), 1200);
    EXPECT_EQ(window.size().height(), 700);
}

TEST_F(QtAppShellTest, ThePagesBecomeTabsInOrder)
{
    ASSERT_NE(shell.Tabs(), nullptr);
    ASSERT_EQ(shell.PageCount(), 3u);
    EXPECT_EQ(shell.Tabs()->tabText(0), "Time Domain");
    EXPECT_EQ(shell.Tabs()->tabText(2), "Impulse Response");
}

TEST_F(QtAppShellTest, ThePanelIsBoundedAndDoesNotStretch)
{
    auto* panel = new QLabel{ "panel" };
    shell.SetPanel(panel);

    EXPECT_EQ(panel->maximumWidth(), 350);
    EXPECT_EQ(shell.Splitter()->widget(0), panel);

    // QSplitter has no stretch getter; it writes the factor through to the child's size policy,
    // which is where the panel-fixed, content-stretches arrangement is observable.
    EXPECT_EQ(panel->sizePolicy().horizontalStretch(), 0);
    EXPECT_EQ(shell.Tabs()->sizePolicy().horizontalStretch(), 1);
}

// The escape hatch: a page is any QWidget, so a hand-written native one needs no wrapper.
TEST_F(QtAppShellTest, APageIsAnyWidgetAndKeepsItsTabTitle)
{
    auto* page = new QLabel{ "chart" };
    shell.SetPage(1, page);

    EXPECT_EQ(shell.Tabs()->widget(1), page);
    EXPECT_EQ(shell.Tabs()->tabText(1), "Frequency Response");
    EXPECT_EQ(shell.PageCount(), 3u);
}

TEST_F(QtAppShellTest, SelectingAPageMovesTheCurrentOne)
{
    shell.SelectPage(2);

    EXPECT_EQ(shell.CurrentPage(), 2u);
}

TEST_F(QtAppShellTest, SelectingAPageBeyondTheEndIsIgnored)
{
    shell.SelectPage(9);

    EXPECT_EQ(shell.CurrentPage(), 0u);
}

TEST_F(QtAppShellTest, TheInitialStatusReachesTheStatusBar)
{
    EXPECT_EQ(window.statusBar()->currentMessage(), "Configure filter parameters and press Compute");
}

TEST_F(QtAppShellTest, SettingTheStatusReplacesTheMessage)
{
    shell.SetStatus("Computing");

    EXPECT_EQ(window.statusBar()->currentMessage(), "Computing");
}

// One window in the consumer repositories shows its content directly; the shell has to carry that
// shape rather than force an empty tab bar on it.
TEST_F(PagelessQtAppShellTest, AShellWithNoPagesBuildsNoTabWidget)
{
    EXPECT_EQ(shell.Tabs(), nullptr);
    EXPECT_EQ(shell.PageCount(), 0u);
}

TEST_F(PagelessQtAppShellTest, ContentGoesStraightIntoTheSplitter)
{
    auto* panel = new QLabel{ "panel" };
    auto* content = new QLabel{ "chart" };

    shell.SetPanel(panel);
    shell.SetContent(content);

    EXPECT_EQ(shell.Splitter()->count(), 2);
    EXPECT_EQ(shell.Splitter()->widget(0), panel);
    EXPECT_EQ(shell.Splitter()->widget(1), content);
}
