#pragma once

#include <QString>
#include <array>
#include <atomic>
#include <string>

namespace ui::backend::qt::update
{
    class IssueLog;

    // Writes a FATAL line to the issue log, and a minidump on Windows, when the process crashes.
    class CrashReporter
    {
    public:
        explicit CrashReporter(IssueLog& log, int keptDumps = 3);
        ~CrashReporter();

        CrashReporter(const CrashReporter&) = delete;
        CrashReporter& operator=(const CrashReporter&) = delete;

        // POSIX: async-signal-safe. Windows: best effort, MiniDumpWriteDump allocates.
        void ReportFatal(const char* reason, void* exceptionPointers = nullptr);

        QString DumpPath() const;

    private:
        void NoteUncleanPreviousRun(IssueLog& log);
        void PruneDumps(int keptDumps) const;
        void Install();
        void Uninstall();
        void AppendLine(const char* reason) const;
        bool WriteDump(void* exceptionPointers) const;

#ifdef _WIN32
        using NativePath = std::wstring;
#else
        using NativePath = std::string;
#endif

        QString directory;
        QString baseName;
        QString dumpPath;
        NativePath nativeLogPath;
        NativePath nativeDumpPath;
        std::string dumpName;
        std::atomic_flag reported;
        std::atomic<bool> written{ false };
        bool installed{ false };
    };
}
