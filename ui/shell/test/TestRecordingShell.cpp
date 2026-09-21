#include "ui/backend/recording/RecordingShell.hpp"
#include <array>
#include <gmock/gmock.h>

namespace
{
    using ui::backend::recording::RecordingShell;
    using ui::shell::PageSpec;
    using ui::shell::ShellSpec;

    constexpr std::array<PageSpec, 3> pages{
        PageSpec{ "Time Domain" },
        PageSpec{ "Frequency Response" },
        PageSpec{ "Impulse Response" }
    };

    class RecordingShellTest
        : public ::testing::Test
    {
    protected:
        ShellSpec spec{ "IIR Filter Simulator", ui::Size{ 1200.0f, 700.0f }, 350.0f, pages, "Configure filter parameters and press Compute" };
        RecordingShell shell{ spec };
    };

    class PagelessShellTest
        : public ::testing::Test
    {
    protected:
        ShellSpec spec{ "LQG Controller Simulator", ui::Size{ 1280.0f, 800.0f }, 350.0f, {}, "Configure LQG parameters and press Compute" };
        RecordingShell shell{ spec };
    };
}

TEST_F(RecordingShellTest, TheSpecIsReadBackAsComposed)
{
    EXPECT_EQ(shell.WindowTitle(), "IIR Filter Simulator");
    EXPECT_NEAR(shell.InitialSize().width, 1200.0f, 1e-4f);
    EXPECT_NEAR(shell.InitialSize().height, 700.0f, 1e-4f);
    EXPECT_NEAR(shell.PanelMaximumWidth(), 350.0f, 1e-4f);
    EXPECT_THAT(shell.PageTitles(), ::testing::ElementsAre("Time Domain", "Frequency Response", "Impulse Response"));
}

TEST_F(RecordingShellTest, TheInitialStatusIsTheFirstStatus)
{
    ASSERT_EQ(shell.Statuses().size(), 1u);
    EXPECT_EQ(shell.Statuses().front(), "Configure filter parameters and press Compute");
}

TEST_F(RecordingShellTest, StatusesAccumulateInOrder)
{
    shell.SetStatus("Computing");
    shell.SetStatus("Done");

    EXPECT_THAT(shell.Statuses(), ::testing::ElementsAre("Configure filter parameters and press Compute", "Computing", "Done"));
}

TEST_F(RecordingShellTest, AnAlertKeepsItsTitleAndMessage)
{
    shell.ShowAlert("Computation Error", "Input size must be greater than or equal to segment size.");

    ASSERT_EQ(shell.Alerts().size(), 1u);
    EXPECT_EQ(shell.Alerts().front().title, "Computation Error");
    EXPECT_EQ(shell.Alerts().front().message, "Input size must be greater than or equal to segment size.");
}

TEST_F(RecordingShellTest, SelectingAPageMovesTheCurrentOne)
{
    shell.SelectPage(2);

    EXPECT_EQ(shell.CurrentPage(), 2u);
}

TEST_F(RecordingShellTest, SelectingAPageBeyondTheEndIsIgnored)
{
    shell.SelectPage(9);

    EXPECT_EQ(shell.CurrentPage(), 0u);
}

// One window in the consumer repositories shows its content directly rather than behind a page
// selector, so an empty pages span is a shape the shell has to carry, not an error.
TEST_F(PagelessShellTest, AShellWithNoPagesReportsNone)
{
    EXPECT_EQ(shell.PageCount(), 0u);
    EXPECT_EQ(shell.CurrentPage(), 0u);
    EXPECT_TRUE(shell.PageTitles().empty());
}

TEST_F(PagelessShellTest, AShellWithNoPagesStillCarriesItsStatusAndTitle)
{
    EXPECT_EQ(shell.WindowTitle(), "LQG Controller Simulator");
    ASSERT_EQ(shell.Statuses().size(), 1u);
    EXPECT_EQ(shell.Statuses().front(), "Configure LQG parameters and press Compute");
}
