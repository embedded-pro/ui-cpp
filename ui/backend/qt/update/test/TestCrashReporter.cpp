#include "ui/backend/qt/update/CrashReporter.hpp"
#include "ui/backend/qt/update/IssueLog.hpp"
#include "gtest/gtest.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
    using ui::backend::qt::update::CrashReporter;
    using ui::backend::qt::update::IssueLog;

    class TestCrashReporter
        : public testing::Test
    {
    public:
        TestCrashReporter()
        {
            QDir{ Directory() }.removeRecursively();
        }

        ~TestCrashReporter() override
        {
            QDir{ Directory() }.removeRecursively();
        }

        // Fixed, not a QTemporaryDir: a death test may re-run the test in a new process.
        static QString Directory()
        {
            const auto* test = testing::UnitTest::GetInstance()->current_test_info();
            return QDir{ QDir::tempPath() }.filePath(QString{ "ui-crash-reporter-" } + test->name());
        }

        static IssueLog::Config LogConfig()
        {
            IssueLog::Config config;
            config.directory = Directory();
            config.baseName = "tool";
            return config;
        }

        static QStringList FileLines(const QString& path)
        {
            QFile file{ path };
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                return {};
            return QString::fromUtf8(file.readAll()).split('\n', Qt::SkipEmptyParts);
        }

        static QString LogPath()
        {
            return QDir{ Directory() }.filePath("tool.log");
        }
    };
}

TEST_F(TestCrashReporter, writes_a_timestamped_fatal_line_on_report)
{
    IssueLog log{ LogConfig() };
    CrashReporter reporter{ log };
    reporter.ReportFatal("Unhandled exception 0xC0000005");

    const auto lines = FileLines(LogPath());
    ASSERT_FALSE(lines.isEmpty());
    const QRegularExpression format{ R"(^\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d\.\d{3}Z \[FATAL\] Crash: Unhandled exception 0xC0000005)" };
    EXPECT_TRUE(format.match(lines.back()).hasMatch()) << lines.back().toStdString();
}

TEST_F(TestCrashReporter, reports_only_the_first_fatal_error)
{
    IssueLog log{ LogConfig() };
    CrashReporter reporter{ log };
    reporter.ReportFatal("first");
    reporter.ReportFatal("second");

    const auto lines = FileLines(LogPath());
    EXPECT_TRUE(lines.back().contains("Crash: first"));
    EXPECT_FALSE(lines.join('\n').contains("second"));
}

TEST_F(TestCrashReporter, notes_a_previous_run_that_did_not_close)
{
    QDir{}.mkpath(Directory());
    QFile marker{ QDir{ Directory() }.filePath("tool.running") };
    ASSERT_TRUE(marker.open(QIODevice::WriteOnly));
    marker.close();

    IssueLog log{ LogConfig() };
    CrashReporter reporter{ log };

    EXPECT_TRUE(log.RecentLines(1).front().contains("[WARN ] The previous run did not close normally"));
}

TEST_F(TestCrashReporter, a_clean_close_leaves_no_marker)
{
    IssueLog log{ LogConfig() };
    {
        CrashReporter reporter{ log };
        EXPECT_TRUE(QFile::exists(QDir{ Directory() }.filePath("tool.running")));
    }

    EXPECT_FALSE(QFile::exists(QDir{ Directory() }.filePath("tool.running")));
    CrashReporter next{ log };
    EXPECT_TRUE(log.RecentLines(10).isEmpty());
}

TEST_F(TestCrashReporter, names_the_dump_beside_the_log)
{
    IssueLog log{ LogConfig() };
    CrashReporter reporter{ log };

    const QFileInfo dump{ reporter.DumpPath() };
    EXPECT_EQ(QDir{ Directory() }.absolutePath(), dump.absolutePath());
    EXPECT_TRUE(dump.fileName().startsWith("tool-"));
    EXPECT_TRUE(dump.fileName().endsWith(".dmp"));
}

TEST_F(TestCrashReporter, keeps_only_the_newest_dumps)
{
    QDir{}.mkpath(Directory());
    for (const auto* name : { "tool-20260101-000000.dmp", "tool-20260102-000000.dmp", "tool-20260103-000000.dmp", "tool-20260104-000000.dmp" })
    {
        QFile dump{ QDir{ Directory() }.filePath(name) };
        ASSERT_TRUE(dump.open(QIODevice::WriteOnly));
    }

    IssueLog log{ LogConfig() };
    CrashReporter reporter{ log, 3 };

    const auto kept = QDir{ Directory() }.entryList({ "tool-*.dmp" }, QDir::Files, QDir::Name);
    ASSERT_EQ(2, kept.size());
    EXPECT_EQ("tool-20260103-000000.dmp", kept[0]);
    EXPECT_EQ("tool-20260104-000000.dmp", kept[1]);
}

TEST_F(TestCrashReporter, a_second_reporter_leaves_the_first_in_charge)
{
    IssueLog log{ LogConfig() };
    CrashReporter first{ log };
    {
        CrashReporter second{ log };
    }

    EXPECT_TRUE(QFile::exists(QDir{ Directory() }.filePath("tool.running")));
}

TEST_F(TestCrashReporter, logs_the_exception_that_reaches_terminate)
{
    EXPECT_DEATH(
        {
            IssueLog log{ LogConfig() };
            CrashReporter reporter{ log };
            try
            {
                throw std::runtime_error{ "mesh index out of range" };
            }
            catch (...)
            {
                std::terminate();
            }
        },
        "");

    EXPECT_TRUE(FileLines(LogPath()).join('\n').contains("[FATAL] Crash: Unhandled C++ exception: mesh index out of range"));
}

TEST_F(TestCrashReporter, ends_the_fatal_line_with_a_newline)
{
    IssueLog log{ LogConfig() };
    CrashReporter reporter{ log };
    reporter.ReportFatal("first");

    QFile file{ LogPath() };
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    EXPECT_TRUE(file.readAll().endsWith("Crash: first\n"));
}

TEST_F(TestCrashReporter, logs_an_abort)
{
    EXPECT_DEATH(
        {
            IssueLog log{ LogConfig() };
            CrashReporter reporter{ log };
            std::abort();
        },
        "");

    EXPECT_TRUE(FileLines(LogPath()).join('\n').contains("[FATAL] Crash: Aborted"));
}

#ifndef _WIN32
TEST_F(TestCrashReporter, logs_a_crash_signal)
{
    EXPECT_DEATH(
        {
            IssueLog log{ LogConfig() };
            CrashReporter reporter{ log };
            std::raise(SIGSEGV);
        },
        "");

    EXPECT_TRUE(FileLines(LogPath()).join('\n').contains("[FATAL] Crash: Segmentation fault"));
}
#else
TEST_F(TestCrashReporter, logs_an_access_violation)
{
    EXPECT_DEATH(
        {
            IssueLog log{ LogConfig() };
            CrashReporter reporter{ log };
            RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, 0, nullptr);
        },
        "");

    EXPECT_TRUE(FileLines(LogPath()).join('\n').contains("[FATAL] Crash: Unhandled exception 0xC0000005"));
}
#endif

#ifdef _WIN32
TEST_F(TestCrashReporter, writes_a_minidump_on_windows)
{
    IssueLog log{ LogConfig() };
    CrashReporter reporter{ log };
    reporter.ReportFatal("test");

    EXPECT_EQ(1, QDir{ Directory() }.entryList({ "tool-*.dmp" }, QDir::Files).size());
    EXPECT_TRUE(FileLines(LogPath()).back().contains("; dump tool-"));
}
#endif
