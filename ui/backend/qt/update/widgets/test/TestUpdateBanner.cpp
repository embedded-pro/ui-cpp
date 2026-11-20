#include "ui/backend/qt/update/widgets/UpdateBanner.hpp"
#include "ui/backend/qt/update/widgets/test/DialogAnswerer.hpp"
#include "gtest/gtest.h"
#include <QPushButton>

namespace
{
    using namespace ui::backend::qt::update;

    class TestUpdateBanner
        : public testing::Test
    {
    public:
        static UpdateState State(UpdatePhase phase)
        {
            UpdateState state;
            state.phase = phase;
            state.release = ReleaseInfo{ ReleaseVersion{ 1, 2, 0 }, "v1.2.0", QUrl{ "https://github.com/example/project/releases/tag/v1.2.0" },
                InstallerAsset{ "example-tool-v1.2.0-linux-x86_64.deb", QUrl{ "https://github.com/example/project/releases/download/v1.2.0/example-tool-v1.2.0-linux-x86_64.deb" }, 2048, QByteArray(32, 'a') }, {} };
            state.automaticInstall = true;
            return state;
        }

        const UpdateProduct product{ "Example Tool", "Example", "1.1.0", {}, {}, {}, "example-tool/1.1.0", "Disconnect the device" };
        UpdateBanner banner{ product };
    };
}

TEST_F(TestUpdateBanner, is_hidden_while_no_update_is_offered)
{
    banner.Render(UpdateState{});

    EXPECT_TRUE(banner.isHidden());
}

TEST_F(TestUpdateBanner, offers_the_new_version_against_the_running_one)
{
    banner.Render(State(UpdatePhase::available));

    EXPECT_FALSE(banner.isHidden());
    EXPECT_EQ("Example Tool 1.2.0 is available — you have 1.1.0.", banner.Message());
    EXPECT_EQ("&Install Update…", banner.PrimaryText());
}

TEST_F(TestUpdateBanner, a_portable_installation_offers_the_installer_download_instead)
{
    auto state = State(UpdatePhase::available);
    state.automaticInstall = false;
    banner.Render(state);

    EXPECT_EQ("Do&wnload Installer", banner.PrimaryText());
}

TEST_F(TestUpdateBanner, a_blocked_install_names_what_the_operator_must_do_first)
{
    auto state = State(UpdatePhase::ready);
    state.blocked = true;
    banner.Render(state);

    EXPECT_EQ("1.2.0 is downloaded and verified. Disconnect the device to install it.", banner.Message());
    EXPECT_FALSE(banner.PrimaryEnabled());

    state.blocked = false;
    banner.Render(state);

    EXPECT_EQ("1.2.0 is downloaded and verified.", banner.Message());
    EXPECT_TRUE(banner.PrimaryEnabled());
}

TEST_F(TestUpdateBanner, installing_names_the_product_that_restarts)
{
    banner.Render(State(UpdatePhase::installing));

    EXPECT_EQ("Installing Example Tool 1.2.0 — Example restarts when it is done.", banner.Message());
    EXPECT_TRUE(banner.PrimaryText().isEmpty());
}

TEST_F(TestUpdateBanner, a_failure_says_why_and_offers_another_try)
{
    auto state = State(UpdatePhase::failed);
    state.failure = "the download did not match its checksum";
    banner.Render(state);

    EXPECT_EQ("Could not update to 1.2.0: the download did not match its checksum.", banner.Message());
    EXPECT_EQ("&Try Again", banner.PrimaryText());
}

TEST_F(TestUpdateBanner, a_download_shows_its_progress_in_megabytes)
{
    auto state = State(UpdatePhase::downloading);
    state.received = 1024 * 1024;
    state.total = 4 * 1024 * 1024;
    banner.Render(state);

    EXPECT_EQ("Downloading 1.2.0 — 1.0 of 4.0 MB", banner.Message());

    state.total = 0;
    banner.Render(state);

    EXPECT_EQ("Downloading 1.2.0 — 1.0 MB", banner.Message());
}

TEST_F(TestUpdateBanner, a_failure_without_automatic_install_offers_the_download_page)
{
    auto state = State(UpdatePhase::failed);
    state.automaticInstall = false;
    state.failure = "no installer for this platform";
    banner.Render(state);

    EXPECT_EQ("Open Do&wnload Page", banner.PrimaryText());
}

TEST_F(TestUpdateBanner, the_primary_button_asks_to_install_or_opens_the_download)
{
    ui::backend::qt::update::test::UrlCatcher browser;
    int installs{ 0 };
    QObject::connect(&banner, &UpdateBanner::InstallRequested, [&installs]()
        {
            ++installs;
        });

    banner.Render(State(UpdatePhase::available));
    banner.findChildren<QPushButton*>().front()->click();

    auto portable = State(UpdatePhase::available);
    portable.automaticInstall = false;
    banner.Render(portable);
    banner.findChildren<QPushButton*>().front()->click();

    EXPECT_EQ(1, installs);
    ASSERT_EQ(1u, browser.urls.size());
    EXPECT_TRUE(browser.urls.front().path().endsWith("example-tool-v1.2.0-linux-x86_64.deb"));
}

TEST_F(TestUpdateBanner, release_notes_open_the_release_page)
{
    ui::backend::qt::update::test::UrlCatcher browser;
    banner.Render(State(UpdatePhase::available));

    for (auto* button : banner.findChildren<QPushButton*>())
        if (button->text() == "&Release Notes")
            button->click();

    ASSERT_EQ(1u, browser.urls.size());
    EXPECT_EQ("/example/project/releases/tag/v1.2.0", browser.urls.front().path());
}
