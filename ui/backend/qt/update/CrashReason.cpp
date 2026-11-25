#include "ui/backend/qt/update/CrashReason.hpp"
#include <csignal>
#include <cstring>
#include <exception>

namespace ui::backend::qt::update::detail
{
    namespace
    {
        void Append(CrashReasonText& text, const char* value)
        {
            const auto used = std::strlen(text.data());
            std::strncat(text.data(), value, text.size() - used - 1);
        }
    }

    CrashReasonText CurrentExceptionReason()
    {
        CrashReasonText reason{};
        const auto current = std::current_exception();
        if (!current)
        {
            Append(reason, "std::terminate without an exception");
            return reason;
        }

        Append(reason, "Unhandled C++ exception");
        try
        {
            std::rethrow_exception(current);
        }
        catch (const std::exception& exception) // NOSONAR: any standard exception names its cause in what()
        {
            Append(reason, ": ");
            Append(reason, exception.what());
        }
        catch (...) // NOSONAR: the crash report must describe every exception type
        {
            Append(reason, " of unknown type");
        }
        return reason;
    }

    const char* SignalName(int signal)
    {
        switch (signal)
        {
            case SIGSEGV:
                return "Segmentation fault";
#ifdef SIGBUS
            case SIGBUS:
                return "Bus error";
#endif
            case SIGFPE:
                return "Floating-point exception";
            case SIGILL:
                return "Illegal instruction";
            case SIGABRT:
                return "Aborted";
            default:
                return "Unknown signal";
        }
    }
}
