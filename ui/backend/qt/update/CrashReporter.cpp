#include "ui/backend/qt/update/CrashReporter.hpp"
#include "ui/backend/qt/update/IssueLog.hpp"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>
#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
// windows.h first: dbghelp.h needs its types.
#include <dbghelp.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace ui::backend::qt::update
{
    namespace
    {
        constexpr std::size_t lineCapacity{ 512 };

        struct Line
        {
            std::array<char, lineCapacity> text{};
            std::size_t size{ 0 };

            void Append(const char* value)
            {
                for (; value != nullptr && *value != '\0' && size + 1 < text.size(); ++value)
                    text[size++] = *value == '\n' ? ' ' : *value;
            }

            void AppendNewline()
            {
                if (size + 1 < text.size())
                    text[size++] = '\n';
            }

            void AppendNumber(std::int64_t value, std::size_t digits)
            {
                if (size + digits + 1 > text.size())
                    return;
                for (auto index = digits; index != 0; --index)
                {
                    text[size + index - 1] = static_cast<char>('0' + value % 10);
                    value /= 10;
                }
                size += digits;
            }
        };

        // Howard Hinnant's civil_from_days: no gmtime, which is not async-signal-safe.
        void AppendUtcTimestamp(Line& line)
        {
            const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            const auto seconds = now / 1000;
            const auto days = seconds / 86400 + 719468;
            const auto era = days / 146097;
            const auto dayOfEra = days - era * 146097;
            const auto yearOfEra = (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096) / 365;
            const auto dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
            const auto monthIndex = (5 * dayOfYear + 2) / 153;
            const auto day = dayOfYear - (153 * monthIndex + 2) / 5 + 1;
            const auto month = monthIndex < 10 ? monthIndex + 3 : monthIndex - 9;
            const auto year = yearOfEra + era * 400 + (month <= 2 ? 1 : 0);
            const auto secondOfDay = seconds % 86400;

            line.AppendNumber(year, 4);
            line.Append("-");
            line.AppendNumber(month, 2);
            line.Append("-");
            line.AppendNumber(day, 2);
            line.Append("T");
            line.AppendNumber(secondOfDay / 3600, 2);
            line.Append(":");
            line.AppendNumber(secondOfDay / 60 % 60, 2);
            line.Append(":");
            line.AppendNumber(secondOfDay % 60, 2);
            line.Append(".");
            line.AppendNumber(now % 1000, 3);
            line.Append("Z");
        }

        struct Handlers
        {
            std::atomic<CrashReporter*> active{ nullptr };
            std::terminate_handler previousTerminate{ nullptr };
#ifdef _WIN32
            LPTOP_LEVEL_EXCEPTION_FILTER previousFilter{ nullptr };
            _invalid_parameter_handler previousInvalidParameter{ nullptr };
            _purecall_handler previousPurecall{ nullptr };
            void (*previousAbort)(int){ nullptr };
#else
            std::array<struct sigaction, 5> previousActions{};
#endif
        };

        Handlers& State()
        {
            constinit static Handlers handlers;
            return handlers;
        }

        void Report(const char* reason, void* exceptionPointers = nullptr)
        {
            if (auto* reporter = State().active.load(); reporter != nullptr)
                reporter->ReportFatal(reason, exceptionPointers);
        }

        [[noreturn]] void OnTerminate()
        {
            Line reason;
            reason.Append("Unhandled C++ exception");
            try
            {
                if (const auto current = std::current_exception())
                    std::rethrow_exception(current);
                reason = {};
                reason.Append("std::terminate without an exception");
            }
            catch (const std::exception& exception)
            {
                reason.Append(": ");
                reason.Append(exception.what());
            }
            catch (...)
            {
                reason.Append(" of unknown type");
            }
            Report(reason.text.data());
            std::abort();
        }

#ifdef _WIN32
        LONG WINAPI OnUnhandledException(EXCEPTION_POINTERS* pointers)
        {
            constexpr char hex[]{ "0123456789ABCDEF" };
            Line reason;
            reason.Append("Unhandled exception 0x");
            const auto code = static_cast<std::uint32_t>(pointers->ExceptionRecord->ExceptionCode);
            for (int shift = 28; shift >= 0; shift -= 4)
                reason.text[reason.size++] = hex[(code >> shift) & 0xF];
            Report(reason.text.data(), pointers);
            return EXCEPTION_EXECUTE_HANDLER;
        }

        void OnAbort(int)
        {
            Report("Aborted");
        }

        void OnInvalidParameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
        {
            Report("Invalid parameter passed to a C runtime function");
            std::abort();
        }

        void OnPurecall()
        {
            Report("Pure virtual function call");
            std::abort();
        }
#else
        constexpr std::array<int, 5> crashSignals{ SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT };

        const char* SignalName(int signal)
        {
            switch (signal)
            {
                case SIGSEGV:
                    return "Segmentation fault";
                case SIGBUS:
                    return "Bus error";
                case SIGFPE:
                    return "Floating-point exception";
                case SIGILL:
                    return "Illegal instruction";
                default:
                    return "Aborted";
            }
        }

        void OnSignal(int signal)
        {
            Report(SignalName(signal));
            std::signal(signal, SIG_DFL);
            std::raise(signal);
        }
#endif
    }

    CrashReporter::CrashReporter(IssueLog& log, int keptDumps)
        : directory{ log.Directory() }
        , baseName{ log.BaseName() }
    {
        const auto name = QString{ "%1-%2.dmp" }.arg(baseName, QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss"));
        dumpPath = QDir{ directory }.filePath(name);
        dumpName = name.toStdString();
#ifdef _WIN32
        nativeLogPath = QDir::toNativeSeparators(log.FilePath()).toStdWString();
        nativeDumpPath = QDir::toNativeSeparators(dumpPath).toStdWString();
#else
        nativeLogPath = QFile::encodeName(log.FilePath()).toStdString();
        nativeDumpPath = QFile::encodeName(dumpPath).toStdString();
#endif
        NoteUncleanPreviousRun(log);
        PruneDumps(keptDumps);
        Install();
    }

    CrashReporter::~CrashReporter()
    {
        if (!installed)
            return;
        Uninstall();
        QFile::remove(QDir{ directory }.filePath(baseName + ".running"));
    }

    void CrashReporter::ReportFatal(const char* reason, void* exceptionPointers)
    {
        // A second crashing thread waits, so the process does not die while the first writes.
        if (reported.test_and_set())
        {
            for (int waited = 0; waited != 200 && !written.load(); ++waited)
                std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });
            return;
        }

        const auto dumped = WriteDump(exceptionPointers);
        Line line;
        AppendUtcTimestamp(line);
        line.Append(" [FATAL] Crash: ");
        line.Append(reason);
        if (dumped)
        {
            line.Append("; dump ");
            line.Append(dumpName.c_str());
        }
        line.AppendNewline();
        AppendLine(line.text.data());
        written.store(true);
    }

    QString CrashReporter::DumpPath() const
    {
        return dumpPath;
    }

    void CrashReporter::NoteUncleanPreviousRun(IssueLog& log)
    {
        QDir{}.mkpath(directory);
        QFile marker{ QDir{ directory }.filePath(baseName + ".running") };
        if (marker.exists())
            log.Write(IssueSeverity::warning, QString{ "The previous run did not close normally, or another instance is running; "
                                                       "see the lines above and any %1-*.dmp in %2" }
                                                  .arg(baseName, QDir::toNativeSeparators(directory)));
        if (marker.open(QIODevice::WriteOnly))
            marker.close();
    }

    void CrashReporter::PruneDumps(int keptDumps) const
    {
        const auto dumps = QDir{ directory }.entryList({ baseName + "-*.dmp" }, QDir::Files, QDir::Name | QDir::Reversed);
        for (auto index = std::max(keptDumps - 1, 0); index < dumps.size(); ++index)
            QFile::remove(QDir{ directory }.filePath(dumps[index]));
    }

    void CrashReporter::Install()
    {
        auto& state = State();
        CrashReporter* none{ nullptr };
        if (!state.active.compare_exchange_strong(none, this))
            return;

        installed = true;
        state.previousTerminate = std::set_terminate(OnTerminate);
#ifdef _WIN32
        state.previousFilter = SetUnhandledExceptionFilter(OnUnhandledException);
        state.previousInvalidParameter = _set_invalid_parameter_handler(OnInvalidParameter);
        state.previousPurecall = _set_purecall_handler(OnPurecall);
        state.previousAbort = std::signal(SIGABRT, OnAbort);
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#else
        struct sigaction action
        {};

        action.sa_handler = OnSignal;
        sigemptyset(&action.sa_mask);
        for (std::size_t index = 0; index != crashSignals.size(); ++index)
            sigaction(crashSignals[index], &action, &state.previousActions[index]);
#endif
    }

    void CrashReporter::Uninstall()
    {
        if (!installed)
            return;

        auto& state = State();
        std::set_terminate(state.previousTerminate);
#ifdef _WIN32
        SetUnhandledExceptionFilter(state.previousFilter);
        _set_invalid_parameter_handler(state.previousInvalidParameter);
        _set_purecall_handler(state.previousPurecall);
        std::signal(SIGABRT, state.previousAbort);
#else
        for (std::size_t index = 0; index != crashSignals.size(); ++index)
            sigaction(crashSignals[index], &state.previousActions[index], nullptr);
#endif
        state.active = nullptr;
    }

    void CrashReporter::AppendLine(const char* text) const
    {
        const auto size = std::strlen(text);
#ifdef _WIN32
        const auto file = CreateFileW(nativeLogPath.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            return;
        DWORD written{ 0 };
        WriteFile(file, text, static_cast<DWORD>(size), &written, nullptr);
        CloseHandle(file);
#else
        const auto file = ::open(nativeLogPath.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (file < 0)
            return;
        [[maybe_unused]] const auto written = ::write(file, text, size);
        ::close(file);
#endif
    }

    bool CrashReporter::WriteDump(void* exceptionPointers) const
    {
#ifdef _WIN32
        const auto file = CreateFileW(nativeDumpPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            return false;
        MINIDUMP_EXCEPTION_INFORMATION exception{ GetCurrentThreadId(), static_cast<EXCEPTION_POINTERS*>(exceptionPointers), FALSE };
        const auto type = static_cast<MINIDUMP_TYPE>(MiniDumpWithThreadInfo | MiniDumpWithIndirectlyReferencedMemory);
        const auto written = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, type,
            exceptionPointers != nullptr ? &exception : nullptr, nullptr, nullptr);
        CloseHandle(file);
        return written != FALSE;
#else
        static_cast<void>(exceptionPointers);
        return false;
#endif
    }
}
