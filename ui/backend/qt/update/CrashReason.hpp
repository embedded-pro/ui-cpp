#pragma once

#include <array>

namespace ui::backend::qt::update::detail
{
    using CrashReasonText = std::array<char, 256>;

    // Describes the exception in flight; call it from std::terminate or a catch block.
    CrashReasonText CurrentExceptionReason();

    const char* SignalName(int signal);
}
