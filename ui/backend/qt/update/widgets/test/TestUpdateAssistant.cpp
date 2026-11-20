#include "ui/backend/qt/update/widgets/UpdateAssistant.hpp"
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
            const auto path = QDir{ temporary.path() }.filePath("latest.json");
            QFile file{ path };
            EXPECT_TRUE(file.open(QIODevice::WriteOnly));
            file.write(QString(R"({ "tag_name": "%1", "html_url": "https://github.com/example/project/releases/tag/%1", "assets": [] })").arg(tag).toUtf8());
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
