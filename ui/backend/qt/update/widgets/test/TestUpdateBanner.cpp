#include "ui/backend/qt/update/widgets/UpdateBanner.hpp"
#include "gtest/gtest.h"

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
