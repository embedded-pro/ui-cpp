#pragma once

#include <QString>
#include <array>
#include <atomic>
#include <cstddef>
#include <string>

struct _EXCEPTION_POINTERS;

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
        void ReportFatal(const char* reason, _EXCEPTION_POINTERS* exceptionPointers = nullptr);

        QString DumpPath() const;

    private:
        void NoteUncleanPreviousRun(IssueLog& log);
        void PruneDumps(int keptDumps) const;
        void Install();
        void Uninstall() const;
        void AppendLine(const char* text, std::size_t size) const;
        bool WriteDump(_EXCEPTION_POINTERS* exceptionPointers) const;

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
