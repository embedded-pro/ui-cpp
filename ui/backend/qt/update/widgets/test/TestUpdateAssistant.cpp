#include "ui/backend/qt/update/widgets/UpdateAssistant.hpp"
#include "ui/backend/qt/update/widgets/test/DialogAnswerer.hpp"
#include "gtest/gtest.h"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QWidget>
#include <vector>

namespace
{
    using namespace ui::backend::qt::update;

    class TestUpdateAssistant
        : public testing::Test
    {
    public:
        TestUpdateAssistant()
        {
            QSettings{}.clear();
            QObject::connect(&assistant, &UpdateAssistant::Report, [this](IssueSeverity, const QString& message)
                {
                    reports.push_back(message);
                });
        }

        ~TestUpdateAssistant() override
        {
            QSettings{}.clear();
        }

        QUrl Feed(const QString& tag)
        {
            return WriteFeed(QString(R"({ "tag_name": "%1", "html_url": "https://github.com/example/project/releases/tag/%1", "assets": [] })").arg(tag).toUtf8());
        }

        QUrl WriteFeed(const QByteArray& content)
        {
            const auto path = QDir{ temporary.path() }.filePath("latest.json");
            QFile file{ path };
            EXPECT_TRUE(file.open(QIODevice::WriteOnly));
            file.write(content);
            return QUrl::fromLocalFile(path);
        }

        void WaitForReport(std::size_t count)
        {
            QElapsedTimer timer;
            timer.start();
            while (reports.size() < count && timer.elapsed() < 5000)
                QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        }

        QTemporaryDir temporary;
        QWidget window;
        UpdateProduct product{ "Example Tool", "Example", "1.1.0", Feed("v1.2.0"), QUrl{ "https://github.com/example/project/releases" }, {}, "example-tool/1.1.0", "Disconnect the device" };
        UpdateAssistant assistant{ product, window };
        std::vector<QString> reports;
    };
}

TEST_F(TestUpdateAssistant, an_automatic_check_offers_a_newer_release_in_the_banner)
{
    assistant.Check(false);
    WaitForReport(1);

    ASSERT_EQ(1u, reports.size());
    EXPECT_EQ("Update available: v1.2.0", reports.front());
    EXPECT_EQ(UpdatePhase::available, assistant.State().phase);
    EXPECT_FALSE(assistant.Banner().isHidden());
}

TEST_F(TestUpdateAssistant, an_automatic_check_does_not_offer_a_skipped_version_again)
{
    QSettings{}.setValue("updates/skippedVersion", "v1.2.0");

    assistant.Check(false);
    WaitForReport(1);

    ASSERT_EQ(1u, reports.size());
    EXPECT_EQ("Update available: v1.2.0 (skipped)", reports.front());
    EXPECT_EQ(UpdatePhase::idle, assistant.State().phase);
}

TEST_F(TestUpdateAssistant, skipping_remembers_the_version_and_hides_the_banner)
{
    assistant.Check(false);
    WaitForReport(1);

    emit assistant.Banner().SkipRequested();

    EXPECT_EQ("v1.2.0", QSettings{}.value("updates/skippedVersion").toString());
    EXPECT_EQ(UpdatePhase::idle, assistant.State().phase);
    EXPECT_TRUE(assistant.Banner().isHidden());
}

TEST_F(TestUpdateAssistant, automatic_checks_are_on_until_turned_off)
{
    EXPECT_TRUE(assistant.Automatic());

    assistant.SetAutomatic(false);

    EXPECT_FALSE(assistant.Automatic());
}

TEST_F(TestUpdateAssistant, blocking_holds_the_session)
{
    assistant.SetBlocked(true);

    EXPECT_TRUE(assistant.State().blocked);
}

TEST_F(TestUpdateAssistant, an_automatic_check_that_finds_no_newer_release_only_reports_it)
{
    Feed("v1.1.0");

    assistant.Check(false);
    WaitForReport(1);

    ASSERT_EQ(1u, reports.size());
    EXPECT_EQ("Up to date (latest release v1.1.0)", reports.front());
    EXPECT_TRUE(assistant.Banner().isHidden());
}

TEST_F(TestUpdateAssistant, an_automatic_check_that_fails_only_reports_it)
{
    QFile::remove(QDir{ temporary.path() }.filePath("latest.json"));

    assistant.Check(false);
    WaitForReport(1);

    ASSERT_EQ(1u, reports.size());
    EXPECT_TRUE(reports.front().startsWith("Update check failed: "));
}

TEST_F(TestUpdateAssistant, a_requested_check_says_so_and_answers_with_a_dialog)
{
    Feed("v1.1.0");
    int checking{ 0 };
    QObject::connect(&assistant, &UpdateAssistant::Checking, [&checking]()
        {
            ++checking;
        });

    ui::backend::qt::update::test::AnswerNextDialog(true);
    assistant.Check(true);
    WaitForReport(1);

    EXPECT_EQ(1, checking);
    ASSERT_EQ(1u, reports.size());
    EXPECT_EQ("Up to date (latest release v1.1.0)", reports.front());
}

TEST_F(TestUpdateAssistant, a_requested_check_that_fails_answers_with_a_dialog)
{
    QFile::remove(QDir{ temporary.path() }.filePath("latest.json"));

    ui::backend::qt::update::test::AnswerNextDialog(true);
    assistant.Check(true);
    WaitForReport(1);

    ASSERT_EQ(1u, reports.size());
    EXPECT_TRUE(reports.front().startsWith("Update check failed: "));
}

TEST_F(TestUpdateAssistant, a_requested_check_offers_even_a_skipped_version)
{
    QSettings{}.setValue("updates/skippedVersion", "v1.2.0");

    assistant.Check(true);
    WaitForReport(1);

    EXPECT_EQ(UpdatePhase::available, assistant.State().phase);
}

TEST_F(TestUpdateAssistant, automatic_checks_start_only_when_turned_on)
{
    assistant.SetAutomatic(false);
    assistant.StartAutomaticChecks();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    EXPECT_TRUE(reports.empty());

    assistant.SetAutomatic(true);
    assistant.StartAutomaticChecks();
    WaitForReport(1);

    EXPECT_EQ(1u, reports.size());
}

TEST_F(TestUpdateAssistant, declining_the_install_confirmation_leaves_the_offer)
{
    assistant.Check(false);
    WaitForReport(1);

    ui::backend::qt::update::test::AnswerNextDialog(false);
    emit assistant.Banner().InstallRequested();

    EXPECT_EQ(UpdatePhase::available, assistant.State().phase);
}

TEST_F(TestUpdateAssistant, downloads_go_to_the_cache_and_the_default_command_follows_the_platform)
{
    EXPECT_TRUE(UpdateAssistant::DownloadDirectory().endsWith("/updates"));
    EXPECT_EQ(DefaultUpdateCommand(CurrentInstallerPlatform(), "/tmp/example.deb", QCoreApplication::applicationFilePath(), QCoreApplication::applicationPid()).has_value(),
        UpdateAssistant::DefaultCommand("/tmp/example.deb").has_value());
}

TEST_F(TestUpdateAssistant, each_update_phase_reports_what_happened)
{
    UpdateState state;
    EXPECT_FALSE(PhaseReport(state).has_value());

    state.release = ReleaseInfo{ ReleaseVersion{ 1, 2, 0 }, "v1.2.0", QUrl{ "https://github.com/example/project/releases/tag/v1.2.0" },
        InstallerAsset{ "example.deb", QUrl{ "https://github.com/example/project/releases/download/v1.2.0/example.deb" }, 1, {} }, {} };
    state.phase = UpdatePhase::downloading;
    EXPECT_EQ("Downloading update v1.2.0 from https://github.com/example/project/releases/download/v1.2.0/example.deb", PhaseReport(state)->message);
    state.phase = UpdatePhase::ready;
    EXPECT_EQ("Update v1.2.0 downloaded and verified", PhaseReport(state)->message);
    state.phase = UpdatePhase::installing;
    EXPECT_EQ("Installing update v1.2.0", PhaseReport(state)->message);
    state.phase = UpdatePhase::failed;
    state.failure = "checksum mismatch";
    EXPECT_EQ(IssueSeverity::warning, PhaseReport(state)->severity);
    EXPECT_EQ("Update to v1.2.0 failed: checksum mismatch", PhaseReport(state)->message);
    state.phase = UpdatePhase::available;
    EXPECT_FALSE(PhaseReport(state).has_value());
}

TEST_F(TestUpdateAssistant, an_issue_report_opens_the_prefilled_new_issue_page)
{
    ui::backend::qt::update::test::UrlCatcher browser;
    product.newIssueUrl = QUrl{ "https://github.com/example/project/issues/new" };
    IssueLog log{ IssueLog::Config{ temporary.path(), "example" } };
    log.Write(IssueSeverity::info, "Session started");

    EXPECT_TRUE(OpenIssueReport(product, log, &window));

    ASSERT_EQ(1u, browser.urls.size());
    EXPECT_EQ("/example/project/issues/new", browser.urls.front().path());
}

TEST_F(TestUpdateAssistant, the_install_explanation_follows_the_platform_and_keeps_the_blocked_action_s_capitals)
{
    product.blockedAction = "Disconnect from the CAN bus";

    EXPECT_EQ("Example downloads the package and checks it against the release, then asks for your password to install it and restarts.",
        InstallExplanation(product, false, InstallerPlatform::linuxX64));
    EXPECT_TRUE(InstallExplanation(product, false, InstallerPlatform::windowsX64).contains("Windows asks for permission"));
    EXPECT_TRUE(InstallExplanation(product, true, InstallerPlatform::linuxX64).endsWith("\n\nInstalling waits until you disconnect from the CAN bus."));
}

TEST_F(TestUpdateAssistant, starting_removes_downloads_older_than_a_day_and_keeps_newer_ones)
{
    QDir{}.mkpath(UpdateAssistant::DownloadDirectory());
    const auto stale = QDir{ UpdateAssistant::DownloadDirectory() }.filePath("stale.deb");
    const auto fresh = QDir{ UpdateAssistant::DownloadDirectory() }.filePath("fresh.deb");
    for (const auto& path : { stale, fresh })
    {
        QFile file{ path };
        ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    }
    QFile staleFile{ stale };
    ASSERT_TRUE(staleFile.open(QIODevice::ReadWrite));
    ASSERT_TRUE(staleFile.setFileTime(QDateTime::currentDateTimeUtc().addDays(-2), QFileDevice::FileModificationTime));
    staleFile.close();

    UpdateAssistant restarted{ product, window };

    EXPECT_FALSE(QFile::exists(stale));
    EXPECT_TRUE(QFile::exists(fresh));
    QFile::remove(fresh);
}
